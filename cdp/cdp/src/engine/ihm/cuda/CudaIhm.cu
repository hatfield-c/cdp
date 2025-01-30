#include "CudaIhm.cuh"

__device__ void CudaIhm::SyncThreads() {
    __syncthreads();
}

__global__ void CudaIhm::SmallestIndexReduction_Kernel(IhmCortex ihm_cortex, int iteration, byte* difference_vector, unsigned long long* index_buffer) {
    void(*func_ptr)() = &CudaIhm::SyncThreads;
    ihm_cortex.IndexReduction(iteration, difference_vector, index_buffer, func_ptr);
}

__global__ void CudaIhm::GetDifferenceVector_Kernel (IhmCortex ihm_cortex, byte* phash, byte* difference_vector, unsigned long long difference_threshold) {
    ihm_cortex.GetDifferenceVector(phash, difference_vector, difference_threshold);
}

__global__ void CudaIhm::GetChamferDistances_Kernel(IhmCortex ihm_cortex, Vector3* cloud, float* distances) {
    ihm_cortex.GetChamferDistances(cloud, distances);
}

__global__ void CudaIhm::GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm) {
    ihm_generator.Generate(space_data, &camera, ihm);
}

__global__ void CudaIhm::ExtractRenderClouds_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm, Vector3* ihm_clouds) {
    ihm_generator.ExtractRenderClouds(space_data, &camera, ihm, ihm_clouds);
}

unsigned long long CudaIhm::FindIhmIndex(IhmCortex ihm_cortex, byte* phash, bool is_verbose) {
    if (is_verbose) {
        printf("\n");
        printf("Querying IHM...\n");
        printf("    Getting difference vector...\n");
    }
    byte* difference_vector = CudaIhm::GetDifferenceVector(ihm_cortex, phash, is_verbose);

    if (is_verbose) {
        printf("    Scan-reducing difference vector...\n");
    }
    unsigned long long smallest_index = CudaIhm::SmallestIndexReduction(ihm_cortex, difference_vector, is_verbose);

    if (is_verbose) {
        printf("    Done!\n");
        printf("\n");
    }

    cudaFree(difference_vector);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    return smallest_index;
}

unsigned long long CudaIhm::SmallestIndexReduction(IhmCortex ihm_cortex, byte* difference_vector, bool is_verbose) {
    //int* top_ten_count = new int[10];
    unsigned long long smallest_index = 0;

    dim3 threads_per_block(32, 1, 1);
    unsigned long long units_per_block = threads_per_block.x * ihm_cortex.thread_units;
    unsigned long long block_count = ceil(ihm_cortex.state_count / units_per_block);
    int iterations_max = ceil(log2(ihm_cortex.state_count) / log2(units_per_block));

    dim3 blocks_per_grid(block_count, 1, 1);

    if (is_verbose) {
        printf("        Units Per Block: %lld\n", units_per_block);
        printf("        Max Iterations: %lld\n", iterations_max);
    }

    unsigned long long* index_buffer;
    int buffer_size = threads_per_block.x * blocks_per_grid.x * sizeof(unsigned long long);
    CudaError::CheckError((cudaError_enum)cudaMalloc(&index_buffer, buffer_size), __FILE__, __LINE__);

    for (int i = 0; i < iterations_max; i++) {
        if (is_verbose) {
            printf("        Iteration: %d\n", i);
            printf("            Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);
        }

        SmallestIndexReduction_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, i, difference_vector, index_buffer);

        block_count = ceil(((float)block_count) / ((float)units_per_block));
        blocks_per_grid = dim3(block_count, 1, 1);

        CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    }

    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMemcpy(&smallest_index, index_buffer, sizeof(unsigned long long), cudaMemcpyDeviceToHost), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    cudaFree(index_buffer);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    return smallest_index;
}

byte* CudaIhm::GetDifferenceVector(IhmCortex ihm_cortex, byte* phash, bool is_verbose, unsigned long long difference_threshold) {
    unsigned long long memory_size = ihm_cortex.state_count * sizeof(byte);

    byte* difference_vector;
    CudaError::CheckError((cudaError_enum)cudaMalloc(&difference_vector, memory_size), __FILE__, __LINE__);
    cudaMemset(difference_vector, 0, memory_size);

    dim3 threads_per_block(32, 1, 1);

    unsigned long long block_count = ceil(ihm_cortex.state_count / threads_per_block.x);

    dim3 blocks_per_grid(block_count, 1, 1);

    if (is_verbose) {
        printf("    Difference Vector\n");
        printf("        Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);
    }

    GetDifferenceVector_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, phash, difference_vector, difference_threshold);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    return difference_vector;
}

float* CudaIhm::GetChamferDistances(IhmCortex ihm_cortex, Vector3* cloud) {
    unsigned long long memory_size = ihm_cortex.state_count * sizeof(float);

    float* distances;
    float* distances_cpu;
    CudaError::CheckError((cudaError_enum)cudaMalloc(&distances, memory_size), __FILE__, __LINE__);
    cudaMemset(distances, 0, memory_size);

    dim3 threads_per_block(32, 1, 1);

    unsigned long long block_count = ceil(ihm_cortex.state_count / threads_per_block.x);

    dim3 blocks_per_grid(block_count, 1, 1);

    printf("    Getting Chamfer Distances:\n");
    printf("        Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);

    GetChamferDistances_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, cloud, distances);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("        Done!");

    return distances;
}

void CudaIhm::GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);

    unsigned long long x_blocks = ihm_generator.state_count;
    unsigned long long y_blocks = ceil(camera.phash_data_size.y / 2);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);

    printf("    Generating IHM:\n");
    printf("        Block Count: (%lld, %lld, %lld)\n", x_blocks, y_blocks, 1);
    printf("        Progress (Max 20 *): ");
    GenerateIhm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, ihm_generator, ihm);
    
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    printf("\n");
}

void CudaIhm::ExtractRenderClouds(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm, Vector3* ihm_clouds) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);
    unsigned long long x_blocks = ihm_generator.state_count;
    int y_blocks = ceil(camera.phash_data_size.y / 2);
    dim3 blocks_per_grid(x_blocks, y_blocks, 1);

    printf("    Extracing IHM point clouds:\n");
    printf("        Block Count: (%lld, %lld, %lld)\n", x_blocks, y_blocks, 1);
    printf("        Progress (Max 20 *): ");
    CudaIhm::ExtractRenderClouds_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, ihm_generator, ihm, ihm_clouds);
    
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    printf("");
}

Vector3* CudaIhm::EstimatePosition(IhmCortex ihm_cortex, IhmGenerator ihm_generator, byte* sensor_phash, Vector3 anchor, int direction_index, bool is_verbose) {
    Vector3 search_radius{ 16, 8, 16 };
    Vector3 search_size = search_radius * 2;
    int voxel_count = search_size.x * search_size.y * search_size.z;

    byte* difference = CudaIhm::GetDifferenceVector(ihm_cortex, sensor_phash, false, 56);
    unsigned long long* index_matrix0;
    unsigned long long* index_matrix1;
    unsigned long long* index_matrix2;
    
    CudaError::CheckError((cudaError_enum)cudaMalloc(&index_matrix0, voxel_count * sizeof(unsigned long long)), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMalloc(&index_matrix1, voxel_count * sizeof(unsigned long long)), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMalloc(&index_matrix2, voxel_count * sizeof(unsigned long long)), __FILE__, __LINE__);

    cudaMemset(index_matrix0, 0, voxel_count * sizeof(unsigned long long));
    cudaMemset(index_matrix1, 0, voxel_count * sizeof(unsigned long long));
    cudaMemset(index_matrix2, 0, voxel_count * sizeof(unsigned long long));
    
    dim3 threads_per_block(32, 1, 1);

    unsigned long long x_blocks = ceil(voxel_count / (float)threads_per_block.x);
    dim3 blocks_per_grid(x_blocks, 1, 1);

    if (is_verbose) {
        printf("    Estimating Position:\n");
        printf("        Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);
        printf("        Progress (Max 20 *): ");
    }
    //FilterLocalOffsets_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, ihm_generator, difference, index_matrix0, index_matrix1, index_matrix2, direction_index, anchor);
    
    unsigned long long* candidate_matrix0 = new unsigned long long[voxel_count];
    unsigned long long* candidate_matrix1 = new unsigned long long[voxel_count];
    unsigned long long* candidate_matrix2 = new unsigned long long[voxel_count];

    memset(candidate_matrix0, 0, voxel_count * sizeof(unsigned long long));
    memset(candidate_matrix1, 0, voxel_count * sizeof(unsigned long long));
    memset(candidate_matrix2, 0, voxel_count * sizeof(unsigned long long));

    CudaError::CheckError((cudaError_enum)cudaMemcpy(candidate_matrix0, index_matrix0, voxel_count * sizeof(unsigned long long), cudaMemcpyDeviceToHost), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMemcpy(candidate_matrix1, index_matrix1, voxel_count * sizeof(unsigned long long), cudaMemcpyDeviceToHost), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMemcpy(candidate_matrix2, index_matrix2, voxel_count * sizeof(unsigned long long), cudaMemcpyDeviceToHost), __FILE__, __LINE__);

    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    Vector3* estimates = new Vector3[3];
    int* nonzero_counts = new int[3];
    memset(estimates, 0, 3 * sizeof(Vector3));
    memset(nonzero_counts, 0, 3 * sizeof(int));

    for (int i = 0; i < search_size.x; i++) {
        for (int j = 0; j < search_size.y; j++) {
            for (int k = 0; k < search_size.z; k++) {
                unsigned long long voxel_index = Indexer::FlatIndex3(i, j, k, search_size.x, search_size.y);
                unsigned long long candidate_index0 = candidate_matrix0[voxel_index];
                unsigned long long candidate_index1 = candidate_matrix1[voxel_index];
                unsigned long long candidate_index2 = candidate_matrix2[voxel_index];

                //printf("[c]: <%d %d %d> (%lld, %lld, %lld)\n", i, j, k, candidate_index0, candidate_index1, candidate_index2);

                if (candidate_index0 != 0) {
                    IhmState ihm_state = ihm_generator.GetIhmState(candidate_index0, false);

                    estimates[0] += ihm_state.position;
                    //printf("[c]: %lld <%.2f %.2f %.2f> (%.2f, %.2f, %.2f)\n", candidate_index0, ihm_state.position.x, ihm_state.position.y, ihm_state.position.z, estimates[0].x, estimates[0].y, estimates[0].z);
                    nonzero_counts[0]++;
                }

                if (candidate_index1 != 0) {
                    IhmState ihm_state = ihm_generator.GetIhmState(candidate_index1, false);

                    //printf("[c]: %lld <%.2f %.2f %.2f> (%.2f, %.2f, %.2f)\n", candidate_index0, ihm_state.position.x, ihm_state.position.y, ihm_state.position.z, estimates[1].x, estimates[1].y, estimates[1].z);

                    estimates[1] += ihm_state.position;
                    nonzero_counts[1]++;
                }

                if (candidate_index2 != 0) {
                    IhmState ihm_state = ihm_generator.GetIhmState(candidate_index2, false);

                    estimates[2] += ihm_state.position;
                    nonzero_counts[2]++;
                }

            }
        }
    }

    //printf("\n<%.2f %.2f %.2f> %d %.2f %.2f\n", estimates[1].x, estimates[1].y, estimates[1].z, nonzero_counts[1], estimates[1].x / nonzero_counts[1], (estimates[1] / nonzero_counts[1]).x);
    //printf("%d, %d, %d\n", nonzero_counts[0], nonzero_counts[1], nonzero_counts[2]);

    if (nonzero_counts[0] > 0) {
        estimates[0] = estimates[0] / nonzero_counts[0];
    }

    if (nonzero_counts[1] > 0) {
        estimates[1] = estimates[1] / nonzero_counts[1];
    }

    if (nonzero_counts[2] > 0) {
        estimates[2] = estimates[2] / nonzero_counts[2];
    }

    //printf("\n<%.2f %.2f %.2f>\n", estimates[1].x, estimates[1].y, estimates[1].z);

    if (is_verbose) {
        printf("\n        Done!\n");
        printf("            Nonzero Count 0: %d\n", nonzero_counts[0]);
        printf("            Nonzero Count 1: %d\n", nonzero_counts[1]);
        printf("            Nonzero Count 2: %d\n", nonzero_counts[2]);
    }

    cudaFree(difference);
    cudaFree(index_matrix0);
    cudaFree(index_matrix1);
    cudaFree(index_matrix2);

    //free(candidate_matrix0);
    //free(candidate_matrix1);
    //free(candidate_matrix2);
    //free(nonzero_counts);

    return estimates;
}
