#pragma once

#include "../Transform.h"

struct IhmState {
	int direction_index;
	Vector3 position{ 0, 0, 0 };
	Vector4 rotation{ 1, 0, 0, 0};
};