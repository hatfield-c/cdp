#pragma once

#include <vector>
#include "cuda.h"

#include "Transform.h"
//#include "Quaternion.h"
#include "../entity/Camera.h"
#include "WorldSpace.h"

class CudaEngine {
	public:
		int cycle_count = 0;
		std::vector<Camera*> camera_list{};
		WorldSpace* world_space;
		void* drone;

		CudaEngine(std::vector<CUdeviceptr> camera_textures);
		void Initialize();
		void Update();
		void Reset();
		void ScenarioUpdate();
		void PhysicsUpdate();
		void RenderUpdate();
		void Cleanup();
};