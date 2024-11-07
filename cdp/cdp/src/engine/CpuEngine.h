#pragma once

#include <vector>
#include "cuda.h"

#include "Transform.h"
#include "Quaternion.h"
#include "../entity/Camera.h"
#include "WorldSpace.h"

class CpuEngine {
	public:
		int cycle_count = 0;
		bool is_simulating = false;
		std::vector<Camera*> camera_list{};
		WorldSpace* world_space;
		void* drone;

		CpuEngine(std::vector<CUdeviceptr> camera_textures);
		void Start();
		void Update();
		void End();
		void ScenarioUpdate();
		void PhysicsUpdate();
		void RenderUpdate();
		void Cleanup();
};