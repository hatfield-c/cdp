#include "CudaIhm.cuh"

__device__ void CudaIhm::SyncThreads() {
    __syncthreads();
}

__global__ void CudaIhm::UpdateNearestDistances_Kernel(IhmCortex ihm_cortex, IhmGenerator ihm_generator, Vector3* camera_cloud, Vector3 anchor) {
    ihm_cortex.UpdateNearestDistances(ihm_generator, camera_cloud, anchor);
}

__global__ void CudaIhm::UpdateChamferDistances_Kernel(IhmCortex ihm_cortex, IhmGenerator ihm_generator) {
    ihm_cortex.UpdateChamferDistances(ihm_generator);
}

__global__ void CudaIhm::GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm) {
    ihm_generator.Generate(space_data, &camera, ihm);
}

__global__ void CudaIhm::ExtractRenderClouds_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm, Vector3* ihm_clouds) {
    ihm_generator.ExtractRenderClouds(space_data, &camera, ihm, ihm_clouds);
}


void CudaIhm::UpdateNearestDistances(IhmCortex ihm_cortex, IhmGenerator ihm_generator, Vector3* camera_cloud, Vector3 anchor) {
    dim3 threads_per_block(32, 1, 1);
    unsigned long long block_count = ceil((float)ihm_cortex.pixel_count / (float)threads_per_block.x);
    dim3 blocks_per_grid(block_count, 1, 1);

    //printf("    Updating Nearest Pixel Distances:\n");
    //printf("        Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);

    UpdateNearestDistances_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, ihm_generator, camera_cloud, anchor);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    //printf("        Done!\n");
}

void CudaIhm::UpdateChamferDistances(IhmCortex ihm_cortex, IhmGenerator ihm_generator, Vector3* camera_cloud, Vector3 anchor) {
    CudaIhm::UpdateNearestDistances(ihm_cortex, ihm_generator, camera_cloud, anchor);

    dim3 threads_per_block(32, 1, 1);
    unsigned long long block_count = ceil((float)ihm_cortex.state_count / ((float)threads_per_block.x));
    dim3 blocks_per_grid(block_count, 1, 1);

    //printf("    Updating Chamfer Distances:\n");
    //printf("        Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);
    UpdateChamferDistances_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, ihm_generator);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    //printf("        Done!\n");
}

void CudaIhm::GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);

    unsigned long long x_blocks = 150000;// ihm_generator.state_count;
    unsigned long long y_blocks = ceil(camera.phash_data_size.y / 2.0f);

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
    int y_blocks = ceil(camera.phash_data_size.y / 2.0);
    dim3 blocks_per_grid(x_blocks, y_blocks, 1);

    printf("    Extracing IHM point clouds:\n");
    printf("        Block Count: (%lld, %lld, %lld)\n", x_blocks, y_blocks, 1);
    printf("        Progress (Max 20 *): ");
    CudaIhm::ExtractRenderClouds_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, ihm_generator, ihm, ihm_clouds);
    
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    printf("\n");
}

