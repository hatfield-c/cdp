#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../SpaceData.h"
#include "../../entity/Camera.h"

namespace CudaCamera {
	__global__ void RenderCamera_Kernel(Camera camera, SpaceData space_data);
	__global__ void DepthUpdate_Kernel(Camera camera, SpaceData space_data);
	void DepthUpdate(Camera camera, SpaceData space_data);
	void RenderCamera(Camera camera, SpaceData space_data);
};