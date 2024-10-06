#include "Tester.cuh"

__global__ void VecAdd(float* A, float* B, float* C, int N)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    //int i = threadIdx.x;// * blockIdx.x + blockDim.x;
    if (i < N)
        C[i] = A[i] + B[i];
}

void VecAddWrapper(float* A, float* B, float* C, int N) {
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
    VecAdd<<<blocksPerGrid, threadsPerBlock>>>(A, B, C, N);
}