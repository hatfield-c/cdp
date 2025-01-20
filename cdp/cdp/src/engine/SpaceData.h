#pragma once

#include <string>

#include "Transform.h"
#include "VoxelData.h"

struct SpaceData {
	const float indices_per_meter = 10;
	Vector3 world_size0{ 1024, 128, 1024};
	Vector3 world_size1{ 512, 64, 512 };
	Vector3 world_size2{ 256, 32, 256 };
	Vector3 level_stride{ 2, 2, 2 };
	unsigned long long voxel_count0;
	unsigned long long voxel_count1;
	unsigned long long voxel_count2;
	unsigned long long memory_size0;
	unsigned long long memory_size1;
	unsigned long long memory_size2;

	VoxelData* space0;
	VoxelData* space1;
	VoxelData* space2;
};