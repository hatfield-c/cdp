#pragma once

#include "Transform.h"

struct VoxelData {
	int entity_id = 0;
	Transform::Vector4 color{ 0, 0, 0, 0 };
};