#include "CudaCamera.cuh"

__device__ void CudaCamera::SyncThreads() {
    __syncthreads();
}

__global__ void CudaCamera::RenderCamera_Kernel(Camera camera, SpaceData space_data)
{
    camera.Render(space_data);
}

__global__ void CudaCamera::BuildCloud_Kernel(Camera camera)
{
    //camera.BuildCloud();
}

void CudaCamera::RenderCamera(Camera camera, SpaceData space_data) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);
    int y_blocks = ceil(camera.phash_data_size.y / 2);
    dim3 blocks_per_grid(1, y_blocks, 1);
    
    //cudaMemset(camera.height_texture, 255, camera.phash_pixel_count * 4 * sizeof(byte));
    //CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    CudaCamera::RenderCamera_Kernel<<<blocks_per_grid, threads_per_block>>>(camera, space_data);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void CudaCamera::BuildCloud(Camera camera) {
    dim3 threads_per_block(camera.phash_data_size.x, 2, 1);
    int y_blocks = ceil(camera.phash_data_size.y / 2);
    dim3 blocks_per_grid(1, y_blocks, 1);

    CudaCamera::BuildCloud_Kernel<<<blocks_per_grid, threads_per_block>>>(camera);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}