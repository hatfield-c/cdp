#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../../../system/CudaError.h"
#include "../../../engine/SpaceData.h"
#include "../Camera.h"

namespace CudaCamera {
	__device__ void SyncThreads();
	__global__ void RenderCamera_Kernel(Camera camera, SpaceData space_data);
	__global__ void BuildCloud_Kernel(Camera camera);
	void RenderCamera(Camera camera, SpaceData space_data);
	void BuildCloud(Camera camera);
};