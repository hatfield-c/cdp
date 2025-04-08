#include "CudaIhm.cuh"

__device__ void CudaIhm::SyncThreads() {
    __syncthreads();
}

__global__ void CudaIhm::GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm) {
    ihm_generator.Generate(space_data, &camera, ihm);
}

__global__ void CudaIhm::GenerateShm_Kernel(SpaceData space_data, IhmGenerator ihm_generator, float* ihm, float* shm, float* buffer) {
    void(*func_ptr)() = &CudaIhm::SyncThreads;
    ihm_generator.GenerateShm(space_data, ihm, shm, buffer, func_ptr);
}

void CudaIhm::GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);

    unsigned long long x_blocks = 100000;//ihm_generator.state_count;
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

void CudaIhm::GenerateShm(SpaceData space_data, IhmGenerator ihm_generator, float* ihm, float* shm) {
    dim3 threads_per_block(24, 1, 1);

    unsigned long long x_blocks = (10 * 10) * 100000;// ihm_generator.state_count;

    dim3 blocks_per_grid(x_blocks, 1, 1);

    float* buffer;
    cudaMalloc(&buffer, ihm_generator.state_count * threads_per_block.x * sizeof(float));

    printf("    Extracting SHM:\n");
    printf("        Block Count: (%lld, %d, %d)\n", x_blocks, 1, 1);
    printf("        Progress (Max 20 *): ");
    GenerateShm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, ihm_generator, ihm, shm, buffer);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}
