#include "CudaIhm.cuh"

__device__ void CudaIhm::SyncThreads() {
    __syncthreads();
}

__global__ void CudaIhm::GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm) {
    ihm_generator.Generate(space_data, &camera, ihm);
}

__global__ void CudaIhm::GenerateShm_Kernel(SpaceData space_data, IhmGenerator ihm_generator, float* ihm, float* shm, float* buffer, float* sums) {
    void(*func_ptr)() = &CudaIhm::SyncThreads;
    ihm_generator.GenerateShm(space_data, ihm, shm, buffer, sums, func_ptr);
}

__global__ void CudaIhm::SummationIhm_Kernel(IhmGenerator ihm_generator, float* ihm, float* sums) {
    ihm_generator.SummationIhm(ihm, sums);
}

__global__ void CudaIhm::SmoothShm_Kernel(IhmGenerator ihm_generator, float* shm, float* buffer) {
    ihm_generator.SmoothShm(shm, buffer);
}

void CudaIhm::GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);

    unsigned long long x_blocks = ihm_generator.state_count;
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

    unsigned long long x_blocks = (10 * 10) * ihm_generator.state_count;

    dim3 blocks_per_grid(x_blocks, 1, 1);

    printf("    Summing heuristics...\n");
    float* sums;
    cudaMalloc(&sums, ihm_generator.state_count * sizeof(float));
    CudaIhm::SummationIhm(ihm_generator, ihm, sums);

    float* buffer;
    cudaMalloc(&buffer, ihm_generator.state_count * threads_per_block.x * sizeof(float));

    printf("    Extracting SHM:\n");
    printf("        Block Count: (%lld, %d, %d)\n", x_blocks, 1, 1);
    printf("        Progress (Max 20 *): ");
    GenerateShm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, ihm_generator, ihm, shm, buffer, sums);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n    Smoothing...\n");
    SmoothShm(ihm_generator, shm);

    printf("\n");
}

void CudaIhm::SummationIhm(IhmGenerator ihm_generator, float* ihm, float* sums) {
    dim3 threads_per_block(32, 1, 1);
    unsigned long long x_blocks = ceil(ihm_generator.state_count / 32);
    dim3 blocks_per_grid(x_blocks, 1, 1);

    SummationIhm_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_generator, ihm, sums);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}

void CudaIhm::SmoothShm(IhmGenerator ihm_generator, float* shm) {
    dim3 threads_per_block(32, 1, 1);
    unsigned long long x_blocks = ceil(ihm_generator.state_count / 32);
    dim3 blocks_per_grid(x_blocks, 1, 1);

    float* buffer;
    cudaMalloc(&buffer, ihm_generator.state_count * 100 * sizeof(float));

    SmoothShm_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_generator, shm, buffer);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}
