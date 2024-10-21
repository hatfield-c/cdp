#include "CudaCamera.cuh"

__device__ int GetIndexCWH(int c, int w, int h, int c_max, int w_max, int h_max) {
    int index = c + (w * c_max) + (h * c_max * w_max);

    return index;
}

__global__ void RenderCamera_Kernel(CameraData camera_data, SpaceData space_data)
{
    //int index = blockDim.x * blockIdx.x + threadIdx.x;

    byte* image_data = camera_data.gpu_texture;

    space_data.space_cuda[100] = VoxelData{ 1, 1 };

    for (int i = 0; i < 640; i++) {
        int index = GetIndexCWH(0, i, 200, 4, 640, 480);
        image_data[index] = 255;

        index = GetIndexCWH(1, i, 280, 4, 640, 480);
        image_data[index] = 255;

        index = GetIndexCWH(2, i, 360, 4, 640, 480);
        image_data[index] = 255;

        index = GetIndexCWH(3, i, 120, 4, 640, 480);
        image_data[index] = 0;
    }

    //if (i < N)
    //    A[i] = -420.69f;
    
}

void RenderCamera(CameraData camera_data, WorldSpace* world_space) {
    //int threadsPerBlock = 32;
    //int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
     
    int threadsPerBlock = 1;
    int blocksPerGrid = 1;

    RenderCamera_Kernel<<<blocksPerGrid, threadsPerBlock>>>(camera_data, world_space->space_data);

}
