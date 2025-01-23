#pragma once

#include "../engine/Transform.h"
#include "VoxelData.h"

struct RaycastHitData {
	Vector3 position{ 0, 0, 0 };
	VoxelData voxel_data{ 0, 0 };
	float distance = 0;
};