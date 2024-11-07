#pragma once

#include <iostream>
#include <string>
#include <vector>

#include"cuda.h"
#include "cuda_runtime.h"
#include "happly.h"

#include "Indexer.h"
#include "SpaceData.h"
#include "VoxelData.h"
#include "../system/CudaError.h"

#include "CudaWorld.cuh"

class WorldSpace {
	public:
		SpaceData space_data;
		
		WorldSpace();
		void InitWorldMemory(bool is_debug_cube, bool is_floor);
		void LoadWorld(std::string load_path);
		void WritePointsToCuda(std::vector<std::array<double, 3>> point_list);
		void SetWorldRegion(Vector3 lower, Vector3 upper, VoxelData voxel_data);
		void Cleanup();
};