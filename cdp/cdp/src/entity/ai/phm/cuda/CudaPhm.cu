#include "CudaPhm.cuh"

__device__ void CudaIhm::SyncThreads() {
    __syncthreads();
}

__global__ void CudaIhm::GeneratePhm_Kernel(SpaceData space_data, Camera camera, PhmGenerator phm_generator, float* phm) {
    phm_generator.Generate(space_data, &camera, phm);
}

__global__ void CudaIhm::GenerateShm_Kernel(SpaceData space_data, PhmGenerator phm_generator, float* phm, float* shm, float* buffer, float* sums) {
    void(*func_ptr)() = &CudaIhm::SyncThreads;
    phm_generator.GenerateShm(space_data, phm, shm, buffer, sums, func_ptr);
}

__global__ void CudaIhm::SummationPhm_Kernel(PhmGenerator phm_generator, float* phm, float* sums) {
    phm_generator.SummationPhm(phm, sums);
}

__global__ void CudaIhm::SmoothShm_Kernel(PhmGenerator phm_generator, float* shm, float* buffer) {
    phm_generator.SmoothShm(shm, buffer);
}

void CudaIhm::GeneratePhm(SpaceData space_data, Camera camera, PhmGenerator phm_generator, float* phm) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);

    unsigned long long x_blocks = phm_generator.state_count;
    unsigned long long y_blocks = ceil(camera.phash_data_size.y / 2.0f);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);

    printf("    Generating PHM:\n");
    printf("        Block Count: (%lld, %lld, %lld)\n", x_blocks, y_blocks, 1);
    printf("        Progress (Max 20 *): ");
    GeneratePhm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, phm_generator, phm);
    
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}

void CudaIhm::GenerateShm(SpaceData space_data, PhmGenerator phm_generator, float* phm, float* shm) {
    dim3 threads_per_block(24, 1, 1);

    unsigned long long x_blocks = (10 * 10) * phm_generator.state_count;

    dim3 blocks_per_grid(x_blocks, 1, 1);

    printf("    Summing heuristics...\n");
    float* sums;
    cudaMalloc(&sums, phm_generator.state_count * sizeof(float));
    CudaIhm::SummationPhm(phm_generator, phm, sums);

    float* buffer;
    cudaMalloc(&buffer, phm_generator.state_count * threads_per_block.x * sizeof(float));

    printf("    Extracting SHM:\n");
    printf("        Block Count: (%lld, %d, %d)\n", x_blocks, 1, 1);
    printf("        Progress (Max 20 *): ");
    GenerateShm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, phm_generator, phm, shm, buffer, sums);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n    Smoothing...\n");
    SmoothShm(phm_generator, shm);

    printf("\n");
}

void CudaIhm::SummationPhm(PhmGenerator phm_generator, float* phm, float* sums) {
    dim3 threads_per_block(32, 1, 1);
    unsigned long long x_blocks = ceil(phm_generator.state_count / 32);
    dim3 blocks_per_grid(x_blocks, 1, 1);

    SummationPhm_Kernel<<<blocks_per_grid, threads_per_block>>>(phm_generator, phm, sums);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}

void CudaIhm::SmoothShm(PhmGenerator phm_generator, float* shm) {
    dim3 threads_per_block(32, 1, 1);
    unsigned long long x_blocks = ceil(phm_generator.state_count / 32);
    dim3 blocks_per_grid(x_blocks, 1, 1);

    float* buffer;
    cudaMalloc(&buffer, phm_generator.state_count * 100 * sizeof(float));

    SmoothShm_Kernel<<<blocks_per_grid, threads_per_block>>>(phm_generator, shm, buffer);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}
