#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../TrainingGenerator.h"

namespace CudaHitPoly {
	__global__ void GenerateTrainingData_Kernel(TrainingGenerator generator);
	void GenerateTrainingData();
}
