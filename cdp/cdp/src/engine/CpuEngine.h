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
#include "ImageBuilder.h"

#include "../ui/GuiData.h"

#include "../entity/camera/Camera.h"
#include "../entity/camera/cuda/CudaCamera.cuh"
#include "../entity/DroneAlpha.h"
#include "../entity/WindGenerator.h"
#include "../entity/ai/hitpoly/NeuralGrid.h"
#include "../entity/ai/hitpoly/cuda/CudaHitpoly.cuh"
#include "../entity/ai/phm/PhmState.h"
#include "../entity/ai/phm/PhmGenerator.h"
#include "../entity/ai/phm/cuda/CudaPhm.cuh"

class CpuEngine {
	public:
		bool is_simulating = false;
		int cycle_count = 0;
		std::chrono::steady_clock::time_point frame_begin_time = std::chrono::steady_clock::now();
		std::vector<Camera*> camera_list{};
		WorldSpace* world_space;
		ImageBuilder* image_builder;
		PhmGenerator phm_generator{};
		DroneAlpha drone_alpha{};
		WindGenerator wind_generatior{};
		byte* simulation_image;

		CpuEngine(std::vector<CUdeviceptr> depth_textures, std::vector<CUdeviceptr> phash_textures, std::vector<CUdeviceptr> derotated_textures);
		void Start(GuiData* gui_data);
		void Update(GuiData* gui_data);
		void End(GuiData* gui_data);
		void ScenarioUpdate(GuiData* gui_data);
		void PhysicsUpdate(GuiData* gui_data);
		void RenderUpdate(GuiData* gui_data);
		void SaveSimulationImage(GuiData* gui_data);
		void DrawDronePosition();
		void SaveProximityHash(GuiData* gui_data);
		void GenerateHitPolyData(GuiData* gui_data);
		void GeneratePhm(GuiData* gui_data);
		void GenerateShm(GuiData* gui_data);
		void Playground(GuiData* gui_data);
		void Cleanup();
};