#pragma once

#include <string>

#include "Transform.h"
#include "VoxelData.h"

struct SpaceData {
	std::string name = "camera";
	const float indices_per_meter = 10;
	Transform::Vector3 world_size{ 1000, 1000, 300 };
	unsigned int voxel_count;

	VoxelData* space;
	VoxelData* space_cuda;
};