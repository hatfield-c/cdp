#include "CudaRender.cuh"

__device__ int GetIndexCWH(int c, int w, int h, int c_max, int w_max, int h_max) {
    int index = c + (w * c_max) + (h * c_max * w_max);

    return index;
}

__global__ void RenderViewport_Kernel(CUdeviceptr viewport_image, byte* A, int N)
{
    //int index = blockDim.x * blockIdx.x + threadIdx.x;

    byte* image_data = (byte*)viewport_image;

    for (int i = 0; i < 32; i++) {
        A[i] = image_data[i];
    }

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

void RenderViewport(CUdeviceptr viewport_image, byte* A, int N) {
    //int threadsPerBlock = 32;
    //int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
     
    int threadsPerBlock = 1;
    int blocksPerGrid = 1;

    RenderViewport_Kernel<<<blocksPerGrid, threadsPerBlock >>>(viewport_image, A, N);

}
