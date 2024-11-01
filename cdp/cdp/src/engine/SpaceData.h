#pragma once

#include <string>

#include "Transform.h"
#include "VoxelData.h"

struct SpaceData {
	const float indices_per_meter = 10;
	Vector3 world_size{ 1000, 300, 1000 };
	unsigned long long voxel_count;
	unsigned long long memory_size;

	VoxelData* space;
	VoxelData* space_cuda;
};