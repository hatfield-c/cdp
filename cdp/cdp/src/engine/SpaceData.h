#pragma once

#include <string>

#include "Transform.h"
#include "VoxelData.h"

struct SpaceData {
	const float indices_per_meter = 10;
	Vector3 world_size0{ 1000, 100, 1000};
	Vector3 world_size1{ (float)floor(1000 / 10), (float)floor(100 / 10), (float)floor(1000 / 10)};
	float level_stride = 10;
	unsigned long long voxel_count0;
	unsigned long long voxel_count1;
	unsigned long long memory_size0;
	unsigned long long memory_size1;

	VoxelData* space0;
	VoxelData* space1;
};