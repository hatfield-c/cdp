#include "CudaIhm.cuh"

__device__ void CudaIhm::SyncThreads() {
    __syncthreads();
}

__global__ void CudaIhm::GetSimilarityScore_Kernel(IhmCortex ihm_cortex, int iteration, byte difference_threshold, byte* difference_vector, double* score_buffer) {
    void(*func_ptr)() = &CudaIhm::SyncThreads;
    ihm_cortex.GetSimilarityScore(iteration, difference_threshold, difference_vector, score_buffer, func_ptr);
}

__global__ void CudaIhm::SmallestIndexReduction_Kernel(IhmCortex ihm_cortex, int iteration, byte* difference_vector, unsigned long long* index_buffer) {
    void(*func_ptr)() = &CudaIhm::SyncThreads;
    ihm_cortex.IndexReduction(iteration, difference_vector, index_buffer, func_ptr);
}

__global__ void CudaIhm::GetDifferenceVector_Kernel (IhmCortex ihm_cortex, byte* phash, byte* difference_vector) {
    ihm_cortex.GetDifferenceVector(phash, difference_vector);
}

__global__ void CudaIhm::GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm) {
    ihm_generator.Generate(space_data, &camera, ihm);
}

__global__ void CudaIhm::RenderHeatmap_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, IhmRenderer ihm_renderer, byte* ihm) {
    //ihm_renderer.RenderHeatmap(space_data, &camera, ihm_generator, ihm);
}

double CudaIhm::GetSimilarityScore(IhmCortex ihm_cortex, byte* phash, bool is_verbose) {
    if (is_verbose) {
        printf("Finding similarity score...\n");
        printf("    Pre-processing...\n");
    }

    double score;

    byte* difference_vector = CudaIhm::GetDifferenceVector(ihm_cortex, phash, is_verbose);
    unsigned long long smallest_index = CudaIhm::SmallestIndexReduction(ihm_cortex, difference_vector, is_verbose);
    byte smallest_value;

    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMemcpy(&smallest_value, difference_vector, sizeof(byte), cudaMemcpyDeviceToHost), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    cudaFree(difference_vector);
    difference_vector = CudaIhm::GetDifferenceVector(ihm_cortex, phash, is_verbose);
    dim3 threads_per_block(32, 1, 1);
    unsigned long long units_per_block = threads_per_block.x * ihm_cortex.thread_units;
    unsigned long long block_count = ceil(ihm_cortex.state_count / units_per_block);
    int iterations_max = ceil(log2(ihm_cortex.state_count) / log2(units_per_block));

    dim3 blocks_per_grid(block_count, 1, 1);

    if (is_verbose) {
        printf("    Calculating score...\n");
        printf("        Units Per Block: %lld\n", units_per_block);
        printf("        Max Iterations: %lld\n", iterations_max);
    }

    double* score_buffer;
    int buffer_size = threads_per_block.x * blocks_per_grid.x * sizeof(unsigned long long);
    CudaError::CheckError((cudaError_enum)cudaMalloc(&score_buffer, buffer_size), __FILE__, __LINE__);

    for (int i = 0; i < iterations_max; i++) {
        if (is_verbose) {
            printf("        Iteration: %d\n", i);
            printf("            Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);
        }

        GetSimilarityScore_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, i, smallest_value, difference_vector, score_buffer);

        block_count = ceil(((float)block_count) / ((float)units_per_block));
        blocks_per_grid = dim3(block_count, 1, 1);

        CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    }

    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMemcpy(&score, score_buffer, sizeof(unsigned long long), cudaMemcpyDeviceToHost), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    cudaFree(difference_vector);
    cudaFree(score_buffer);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    return score;
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

byte* CudaIhm::GetDifferenceVector(IhmCortex ihm_cortex, byte* phash, bool is_verbose) {
    unsigned long long memory_size = ihm_cortex.state_count * sizeof(byte);

    byte* difference_vector;
    CudaError::CheckError((cudaError_enum)cudaMalloc(&difference_vector, memory_size), __FILE__, __LINE__);

    dim3 threads_per_block(32, 1, 1);

    unsigned long long block_count = ceil(ihm_cortex.state_count / threads_per_block.x);

    dim3 blocks_per_grid(block_count, 1, 1);

    if (is_verbose) {
        printf("    Difference Vector\n");
        printf("        Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);
    }

    GetDifferenceVector_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, phash, difference_vector);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    return difference_vector;
}

void CudaIhm::GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm) {
    Vector2 resolution = camera.phash_data_size;

    dim3 threads_per_block(1, 8, 4);

    unsigned long long image_count = ihm_generator.state_count;
    unsigned long long x_blocks = ceil(resolution.x / (float)threads_per_block.y);
    unsigned long long y_blocks = ceil(resolution.y / (float)threads_per_block.z);

    dim3 blocks_per_grid(image_count, x_blocks, y_blocks);

    printf("        Block Count: (%lld, %lld, %lld)\n", image_count, x_blocks, y_blocks);
    printf("        Progress (Max 16 *): ");
    GenerateIhm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, ihm_generator, ihm);
    
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    printf("\n");
}

void CudaIhm::RenderHeatmap(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, IhmRenderer ihm_renderer, byte* ihm, byte* img) {
    Vector2 resolution = camera.phash_data_size;

    dim3 threads_per_block(1, 8, 4);

    unsigned long long image_count = ihm_renderer.pixel_count;
    unsigned long long x_blocks = ceil(resolution.x / (float)threads_per_block.y);
    unsigned long long y_blocks = ceil(resolution.y / (float)threads_per_block.z);

    dim3 blocks_per_grid(image_count, x_blocks, y_blocks);

    printf("        Block Count: (%lld, %lld, %lld)\n", image_count, x_blocks, y_blocks);
    printf("        Progress (Max 16 *): ");
    RenderHeatmap_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, ihm_generator, ihm_renderer, ihm);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    printf("\n");
}
