#include "CudaCamera.cuh"

__device__ int GetIndexCWH(int c, int w, int h, int c_max, int w_max, int h_max) {
    int index = c + (w * c_max) + (h * c_max * w_max);

    return index;
}

__global__ void RenderCamera_Kernel(CameraData camera_data, SpaceData space_data)
{
    int x_index = blockDim.x * blockIdx.x + threadIdx.x;
    int y_index = blockDim.y * blockIdx.y + threadIdx.y;

    if(x_index >= camera_data.resolution.x || y_index >= camera_data.resolution.y){
        return;
    }

    byte* image_data = camera_data.gpu_texture;

    int gpu_index_r = GetIndexCWH(0, x_index, y_index, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_g = GetIndexCWH(1, x_index, y_index, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_b = GetIndexCWH(2, x_index, y_index, 4, camera_data.resolution.x, camera_data.resolution.y);

    image_data[gpu_index_r] = 255;
    image_data[gpu_index_g] = 0;
    image_data[gpu_index_b] = 0;
    
    //space_data.space_cuda[100] = VoxelData{ 1, 1 };

}

void RenderCamera(CameraData camera_data, WorldSpace* world_space) {
    
    Transform::Vector2 resolution = camera_data.resolution;
    
    dim3 threads_per_block(16, 16, 1);

    int x_blocks = ceil(resolution.x / threads_per_block.x);
    int y_blocks = ceil(resolution.y / threads_per_block.y);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);
    
    RenderCamera_Kernel<<<blocks_per_grid, threads_per_block >>>(camera_data, world_space->space_data);
}
