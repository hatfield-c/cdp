#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../../../system/CudaError.h"
#include "../PolyFieldGenerator.h"

namespace CudaPolyField {
	__global__ void GenerateTrainingData_Kernel(PolyFieldGenerator data_generator);
	void GenerateTrainingData();
}
