#pragma once

#include "../../../engine/Transform.h"

struct PhmState {
	int direction_index;
	Vector3 position{ 0, 0, 0 };
	Vector3 position_strided{ 0, 0, 0 };
	Vector4 rotation{ 1, 0, 0, 0};
};