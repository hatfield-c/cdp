#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

__global__ void RenderViewport_Kernel(float* A, int N);

void RenderViewport(float* A, int N);