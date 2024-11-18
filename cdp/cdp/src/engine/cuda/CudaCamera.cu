#include "CudaCamera.cuh"

__global__ void CudaCamera::DepthUpdate_Kernel(Camera camera, SpaceData space_data)
{
    camera.DepthUpdate(space_data);
}

__global__ void CudaCamera::RenderCamera_Kernel(Camera camera, SpaceData space_data)
{
    camera.Render(space_data);
}

void CudaCamera::DepthUpdate(Camera camera, SpaceData space_data) {

    Vector2 resolution = camera.camera_size;

    dim3 threads_per_block(8, 4, 1);

    int x_blocks = ceil(resolution.x / (float)threads_per_block.x);
    int y_blocks = ceil(resolution.y / (float)threads_per_block.y);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);

    CudaCamera::DepthUpdate_Kernel<<<blocks_per_grid, threads_per_block>>>(camera, space_data);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void CudaCamera::RenderCamera(Camera camera, SpaceData space_data) {
    
    Vector2 resolution = camera.camera_size;
    
    dim3 threads_per_block(8, 4, 1);

    int x_blocks = ceil(resolution.x / (float)threads_per_block.x);
    int y_blocks = ceil(resolution.y / (float)threads_per_block.y);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);
    
    CudaCamera::RenderCamera_Kernel<<<blocks_per_grid, threads_per_block>>>(camera, space_data);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}
