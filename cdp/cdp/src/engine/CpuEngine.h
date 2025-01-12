#pragma once

#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <thread>
#include "cuda.h"
#include "stb_image_write.h"

#include "Transform.h"
#include "Quaternion.h"
#include "WorldSpace.h"
#include "../entity/Camera.h"
#include "../entity/DroneAlpha.h"

#include "ihm/IhmState.h"
#include "ihm/IhmGenerator.h"
#include "ihm/IhmCortex.h"
#include "../ui/GuiData.h"

#include "cuda/CudaCamera.cuh"
#include "ihm/cuda/CudaIhm.cuh"

class CpuEngine {
	public:
		int cycle_count = 0;
		std::chrono::steady_clock::time_point frame_begin_time = std::chrono::steady_clock::now();
		bool is_simulating = false;
		std::vector<Camera*> camera_list{};
		WorldSpace* world_space;
		IhmGenerator ihm_generator{};
		IhmCortex ihm_cortex{};
		DroneAlpha drone_alpha{};

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
		void SaveSimilarityHeatMap(GuiData gui_data);
		void EstimatePositionIhm(GuiData gui_data);
		void Cleanup();
};