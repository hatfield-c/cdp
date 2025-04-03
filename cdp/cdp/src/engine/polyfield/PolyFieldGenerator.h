#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../Transform.h"

struct PolyFieldGenerator {
	unsigned long long position_count;
	unsigned long long direction_count;
	unsigned long long pixel_count;
	unsigned long long state_count;
	unsigned long long float_count;

	Vector3 depth_size{ 16, 16 };
	Vector3 position_lower{ 0.0f, 5.0f, 0.0f };
	Vector3 position_upper{ 100.0f, 7.0f, 100.0f };
	int yaw_steps = 8;
	int pitch_steps = 3;
	float pitch_lower = -Math::Pi() / 4;
	float pitch_upper = 0.0f;

	void Init() {
		this->position_count = (unsigned long long)(this->position_upper - this->pitch_lower).Mult();
		this->direction_count = this->yaw_steps * this->pitch_steps;
		this->pixel_count = (unsigned long long)this->depth_size.Mult();
		this->state_count = this->position_count * this->direction_count;
		this->float_count = this->state_count * this->pixel_count;
	}

	__device__ void GenerateData() {

	}
};