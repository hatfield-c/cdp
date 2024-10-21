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
		
		WorldSpace();
		void LoadWorldVoxels();
		void SetWorldRegion(std::vector<int> v0, std::vector<int> v1, VoxelData voxel_data);
		void CheckCudaError(cudaError_enum result, const char* file, int line);
		void Cleanup();
};