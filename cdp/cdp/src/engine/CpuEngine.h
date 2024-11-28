#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include "cuda.h"
#include "SimpleBinStream.h"

#include "Transform.h"
#include "Quaternion.h"
#include "WorldSpace.h"
#include "../entity/Camera.h"
#include "ihm/IhmState.h"
#include "ihm/IhmGenerator.h"
#include "../ui/GuiData.h"

#include "cuda/CudaCamera.cuh"
#include "ihm/cuda/CudaIhm.cuh"

class CpuEngine {
	public:
		int cycle_count = 0;
		bool is_simulating = false;
		std::vector<Camera*> camera_list{};
		WorldSpace* world_space;
		IhmGenerator ihm_generator{};
		byte* ihm_cpu = new byte[1];
		byte* ihm = new byte[1];

		CpuEngine(std::vector<CUdeviceptr> depth_textures, std::vector<CUdeviceptr> phash_textures, std::vector<CUdeviceptr> shaded_textures);
		void Start(GuiData gui_data);
		void Update(GuiData gui_data);
		void End(GuiData gui_data);
		void ScenarioUpdate(GuiData gui_data);
		void PhysicsUpdate(GuiData gui_data);
		void RenderUpdate(GuiData gui_data);
		void GenerateIhm(GuiData gui_data);
		void LoadIhm(GuiData gui_data);
		void VerifyIhm(GuiData gui_data);
		void Cleanup();
};