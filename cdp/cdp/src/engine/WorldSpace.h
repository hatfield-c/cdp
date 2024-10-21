#pragma once

#include <vector>
#include"cuda.h"
#include "cuda_runtime.h"

#include "index.hpp"

#include "SpaceData.h"
#include "VoxelData.h"

class WorldSpace {
	public:
		SpaceData space_data;
		VoxelData* space;
		VoxelData* space_cuda;

		WorldSpace();
		void LoadWorldVoxels();
		void SetWorldRegion(std::vector<int> v0, std::vector<int> v1, VoxelData voxel_data);
		void Cleanup();
};