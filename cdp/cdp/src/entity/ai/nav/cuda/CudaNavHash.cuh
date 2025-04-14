#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../../../../system/CudaError.h"
#include "../../../../engine/SpaceData.h"
#include "../../../camera/Camera.h"
#include "../NavGenerator.h"
#include "../CentroidCortex.h"

namespace CudaNavHash {
	__device__ void SyncThreads();
	__global__ void GeneratePhm_Kernel(SpaceData space_data, Camera camera, NavGenerator nav_generator, float* phm);
	__global__ void GenerateShm_Kernel(SpaceData space_data, NavGenerator nav_generator, CentroidCortex centroid_cortex, float* phm, float* shm);
	__global__ void SummationPhm_Kernel(NavGenerator nav_generator, float* phm, float* sums);
	__global__ void SmoothShm_Kernel(NavGenerator nav_generator, float* shm, float* buffer);
	void GeneratePhm(SpaceData space_data, Camera camera, NavGenerator nav_generator, float* phm);
	void GenerateShm(SpaceData space_data, NavGenerator nav_generator, CentroidCortex centroid_cortex, float* phm, float* shm);
	void SummationPhm(NavGenerator nav_generator, float* phm, float* sums);
	void SmoothShm(NavGenerator nav_generator, float* shm);
}
