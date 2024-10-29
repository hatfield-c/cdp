#pragma once

#include "../engine/Transform.h"
#include "VoxelData.h"

struct RaycastHitData {
	Transform::Vector3 position{};
	VoxelData voxel_data{};
	float distance;
};