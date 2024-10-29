#pragma once

#include"cuda.h"
#include "cuda_runtime.h"

#include "SpaceData.h"
#include "VoxelData.h"

class WorldSpace {
	public:
		SpaceData space_data;
		
		WorldSpace();
		void LoadWorldVoxels();
		void SetWorldRegion(Transform::Vector3 lower, Transform::Vector3 upper, VoxelData voxel_data);
		void CheckCudaError(cudaError_enum result, const char* file, int line);
		int GetIndexCWH(int c, int w, int h, int c_max, int w_max, int h_max);
		void Cleanup();
};