#pragma once

#include <vector>
#include "cuda.h"

#include "../entity/Camera.h"
#include "WorldSpace.h"

class CudaEngine {
	public:
		std::vector<Camera*> camera_list{};
		WorldSpace* world_space;
		void* drone;

		CudaEngine(std::vector<CUdeviceptr> camera_textures);
		void Update();
		void ScenarioUpdate();
		void PhysicsUpdate();
		void RenderUpdate();
		void Cleanup();
};