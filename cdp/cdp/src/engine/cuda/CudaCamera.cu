#include "CudaCamera.cuh"

__device__ void CudaCamera::SyncThreads() {
    __syncthreads();
}

__global__ void CudaCamera::RenderCamera_Kernel(Camera camera, SpaceData space_data)
{
    camera.Render(space_data);
}

__global__ void CudaCamera::GenerateHmeans_Kernel(Camera camera) {
    void(*func_ptr)() = &CudaCamera::SyncThreads;
    camera.GenerateHmeans(func_ptr);
}

void CudaCamera::RenderCamera(Camera camera, SpaceData space_data) {
    
    Vector2 resolution = camera.camera_size;
    
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);

    int y_blocks = ceil(camera.phash_data_size.y / 2);

    dim3 blocks_per_grid(1, y_blocks, 1);
    
    CudaCamera::RenderCamera_Kernel<<<blocks_per_grid, threads_per_block>>>(camera, space_data);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void CudaCamera::GenerateHmeans(Camera camera) {
    dim3 threads_per_block(1, 32, 1);
    dim3 blocks_per_grid(1, (int)(256.0 / 32.0), 1);

    CudaCamera::GenerateHmeans_Kernel<<<blocks_per_grid, threads_per_block>>>(camera);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}