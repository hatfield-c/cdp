#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <windows.h>

__device__ int GetIndex3(int i, int j, int k);

__global__ void RenderViewport_Kernel(CUdeviceptr viewport_image, byte* A, int N);

void RenderViewport(CUdeviceptr viewport_image, byte* A, int N);