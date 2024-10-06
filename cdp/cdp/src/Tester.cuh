#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

__global__ void VecAdd(float* A, float* B, float* C, int N);

void VecAddWrapper(float* A, float* B, float* C, int N);