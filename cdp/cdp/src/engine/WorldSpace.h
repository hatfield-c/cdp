#pragma once

#include <iostream>

#include"cuda.h"
#include "cuda_runtime.h"

#include "Indexer.h"
#include "SpaceData.h"
#include "VoxelData.h"
#include "../system/CudaError.h"

#include "CudaWorld.cuh"

class WorldSpace {
	public:
		SpaceData space_data;
		
		WorldSpace();
		void InitWorldVoxels(bool is_debug_cube);
		void SetWorldRegion(Vector3 lower, Vector3 upper, VoxelData voxel_data);
		void Cleanup();
};