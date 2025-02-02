#pragma once

#include "../Transform.h"

struct IhmEstimate {
	unsigned long long local_index;
	float chamfer_score = 1000000000;
	int direction_index;
	Vector3 position;
};