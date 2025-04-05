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
#include "../entity/WindGenerator.h"
#include "ImageBuilder.h"
#include "ihm/hitpoly/NeuralGrid.h"

#include "ihm/IhmState.h"
#include "ihm/IhmGenerator.h"
#include "ihm/IhmCortex.h"
#include "../ui/GuiData.h"

#include "cuda/CudaCamera.cuh"
#include "ihm/hitpoly/cuda/CudaHitPoly.cuh"
#include "ihm/cuda/CudaIhm.cuh"
#include "ihm/IhmEstimate.h"

class CpuEngine {
	public:
		bool is_simulating = false;
		int cycle_count = 0;
		std::chrono::steady_clock::time_point frame_begin_time = std::chrono::steady_clock::now();
		std::vector<Camera*> camera_list{};
		WorldSpace* world_space;
		ImageBuilder* image_builder;
		IhmGenerator ihm_generator{};
		IhmCortex ihm_cortex{};
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
		void GeneratePolyFieldData(GuiData* gui_data);
		void Playground(GuiData* gui_data);
		void Cleanup();
};