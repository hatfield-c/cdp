#include "CudaRender.cuh"

__global__ void RenderViewport_Kernel(float* A, int N)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    
    if (i < N)
        A[i] = -420.69f;
}

void RenderViewport(float* A, int N) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
    RenderViewport_Kernel<<<blocksPerGrid, threadsPerBlock>>>(A, N);
}