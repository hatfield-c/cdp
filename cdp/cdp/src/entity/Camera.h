#pragma once

#include <string>
#include <vector>
#include "cuda.h"

#include "../engine/WorldSpace.h"
#include "../render/CudaCamera.cuh"
#include "CameraData.h"

class Camera {
	public:
		CameraData camera_data{};
		std::string name;

		Camera(std::string name, CUdeviceptr gpu_texture);
		void Render(WorldSpace* world_space);
};