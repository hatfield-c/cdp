#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../../SpaceData.h"
#include "../../../entity/Camera.h"
#include "../IhmGenerator.h"
#include "../IhmRenderer.h"
#include "../IhmCortex.h"

namespace CudaIhm {
	__device__ void SyncThreads();
	__global__ void GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm);
	__global__ void GenerateShm_Kernel(SpaceData space_data, IhmGenerator ihm_generator, float* ihm, float* shm, float* buffer, float* sums);
	__global__ void SummationIhm_Kernel(IhmGenerator ihm_generator, float* ihm, float* sums);
	__global__ void SmoothShm_Kernel(IhmGenerator ihm_generator, float* shm, float* buffer);
	void GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, float* ihm);
	void GenerateShm(SpaceData space_data, IhmGenerator ihm_generator, float* ihm, float* shm);
	void SummationIhm(IhmGenerator ihm_generator, float* ihm, float* sums);
	void SmoothShm(IhmGenerator ihm_generator, float* shm);
}
