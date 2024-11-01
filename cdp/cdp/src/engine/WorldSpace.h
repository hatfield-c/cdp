#pragma once

#include"cuda.h"
#include "cuda_runtime.h"

#include "Indexer.h"
#include "SpaceData.h"
#include "VoxelData.h"

class WorldSpace {
	public:
		SpaceData space_data;
		
		WorldSpace();
		void LoadWorldVoxels();
		void SetWorldRegion(Vector3 lower, Vector3 upper, VoxelData voxel_data);
		void CheckCudaError(cudaError_enum result, const char* file, int line);
		void Cleanup();
};