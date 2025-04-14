#include "CudaNavHash.cuh"

__device__ void CudaNavHash::SyncThreads() {
    __syncthreads();
}

__global__ void CudaNavHash::GeneratePhm_Kernel(SpaceData space_data, Camera camera, NavGenerator nav_generator, float* phm) {
    nav_generator.GeneratePhm(space_data, &camera, phm);
}

__global__ void CudaNavHash::GenerateShm_Kernel(SpaceData space_data, NavGenerator nav_generator, CentroidCortex centroid_cortex, float* phm, float* shm) {
    void(*func_ptr)() = &CudaNavHash::SyncThreads;
    nav_generator.GenerateShm(space_data, centroid_cortex, phm, shm);
}

__global__ void CudaNavHash::SummationPhm_Kernel(NavGenerator nav_generator, float* phm, float* sums) {
    nav_generator.SummationPhm(phm, sums);
}

__global__ void CudaNavHash::SmoothShm_Kernel(NavGenerator nav_generator, float* shm, float* buffer) {
    nav_generator.SmoothShm(shm, buffer);
}

void CudaNavHash::GeneratePhm(SpaceData space_data, Camera camera, NavGenerator nav_generator, float* phm) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);

    unsigned long long x_blocks = nav_generator.state_count;
    unsigned long long y_blocks = ceil(camera.phash_data_size.y / 2.0f);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);

    printf("    Generating PHM:\n");
    printf("        Block Count: (%lld, %lld, %lld)\n", x_blocks, y_blocks, 1);
    printf("        Progress (Max 20 *): ");
    GeneratePhm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, nav_generator, phm);
    
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}

void CudaNavHash::GenerateShm(SpaceData space_data, NavGenerator nav_generator, CentroidCortex centroid_cortex, float* phm, float* shm) {
    dim3 threads_per_block(32, 1, 1);

    unsigned long long x_blocks = (100 * 100) * centroid_cortex.centroid_count;
    x_blocks = ceil(x_blocks / threads_per_block.x);

    dim3 blocks_per_grid(x_blocks, 1, 1);

    //printf("    Summing heuristics...\n");
    //float* sums;
    //cudaMalloc(&sums, nav_generator.state_count * sizeof(float));
    //CudaNavHash::SummationPhm(nav_generator, phm, sums);

    //float* buffer;
    //cudaMalloc(&buffer, nav_generator.state_count * threads_per_block.x * sizeof(float));

    printf("    Extracting SHM:\n");
    printf("        Block Count: (%lld, %d, %d)\n", x_blocks, 1, 1);
    printf("        Progress (Max 20 *): ");
    GenerateShm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, nav_generator, centroid_cortex, phm, shm);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    //printf("\n    Smoothing...\n");
    //SmoothShm(nav_generator, shm);

    printf("\n");
}

void CudaNavHash::SummationPhm(NavGenerator nav_generator, float* phm, float* sums) {
    dim3 threads_per_block(32, 1, 1);
    unsigned long long x_blocks = ceil(nav_generator.state_count / 32);
    dim3 blocks_per_grid(x_blocks, 1, 1);

    SummationPhm_Kernel<<<blocks_per_grid, threads_per_block>>>(nav_generator, phm, sums);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}

void CudaNavHash::SmoothShm(NavGenerator nav_generator, float* shm) {
    dim3 threads_per_block(32, 1, 1);
    unsigned long long x_blocks = ceil(256 / 32);
    dim3 blocks_per_grid(x_blocks, 1, 1);

    float* buffer;
    cudaMalloc(&buffer, 256 * 100 * 100 * sizeof(float));

    SmoothShm_Kernel<<<blocks_per_grid, threads_per_block>>>(nav_generator, shm, buffer);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    printf("\n");
}
