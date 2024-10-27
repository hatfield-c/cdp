#pragma once

#include "../engine/Transform.h"
#include "VoxelData.h"

struct RaycastHitData {
	Transform::Vector3 hit_position{};
	VoxelData hit_voxel_data{};
	float distance_traveled;
};