#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <windows.h>

__global__ void RenderViewport_Kernel(CUdeviceptr viewport_image, byte* A, int N);

void RenderViewport(CUdeviceptr viewport_image, byte* A, int N);