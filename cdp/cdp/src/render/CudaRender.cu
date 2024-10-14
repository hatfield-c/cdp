#include "CudaRender.cuh"

__global__ void RenderViewport_Kernel(CUdeviceptr viewport_image, byte* A, int N)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    byte* test = (byte*)viewport_image;

    A[0] = test[198];
    A[1] = test[199];
    A[2] = test[200];
    A[3] = test[201];
    A[4] = test[202];

    //if (i < N)
    //    A[i] = -420.69f;
}

void RenderViewport(CUdeviceptr viewport_image, byte* A, int N) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
    RenderViewport_Kernel<<<blocksPerGrid, threadsPerBlock >>>(viewport_image, A, N);

}
/*__global__ void RenderViewport_Kernel(float* A, int N)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    
    if (i < N)
        A[i] = -420.69f;
}

void RenderViewport(float* A, int N) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
    RenderViewport_Kernel<<<blocksPerGrid, threadsPerBlock>>>(A, N);
}*/