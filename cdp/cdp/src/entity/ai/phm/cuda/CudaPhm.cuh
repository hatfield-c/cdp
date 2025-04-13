#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../../../../system/CudaError.h"
#include "../../../../engine/SpaceData.h"
#include "../../../camera/Camera.h"
#include "../PhmGenerator.h"

namespace CudaIhm {
	__device__ void SyncThreads();
	__global__ void GeneratePhm_Kernel(SpaceData space_data, Camera camera, PhmGenerator ihm_generator, float* ihm);
	__global__ void GenerateShm_Kernel(SpaceData space_data, PhmGenerator ihm_generator, float* ihm, float* shm, float* buffer, float* sums);
	__global__ void SummationPhm_Kernel(PhmGenerator phm_generator, float* ihm, float* sums);
	__global__ void SmoothShm_Kernel(PhmGenerator phm_generator, float* shm, float* buffer);
	void GeneratePhm(SpaceData space_data, Camera camera, PhmGenerator phm_generator, float* ihm);
	void GenerateShm(SpaceData space_data, PhmGenerator phm_generator, float* ihm, float* shm);
	void SummationPhm(PhmGenerator phm_generator, float* ihm, float* sums);
	void SmoothShm(PhmGenerator phm_generator, float* shm);
}
