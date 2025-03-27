#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../../../engine/WorldSpace.h"
#include "../../../engine/Physics.h"
#include "../../../engine/Transform.h"
#include "../../../engine/Quaternion.h"
#include "../../../engine/Indexer.h"

struct TrainingGenerator {

	unsigned long long state_count;
	unsigned long long float_count;
	unsigned long long position_count;
	unsigned long long velocity_count;
	int dim = 6;
	Vector3 position_lower{};
	Vector3 position_steps{};
	Vector3 velocity_lower{};
	Vector3 velocity_steps{};
	int time_steps = (1 / Physics::DeltaTime()) * 2;

	float* state_data;
	float* value_data;

	void Init(Vector3 position_lower, Vector3 position_steps, Vector3 velocity_lower, Vector3 velocity_steps) {
		this->position_lower = position_lower;
		this->position_steps = position_steps;
		this->velocity_lower = velocity_lower;
		this->velocity_steps = velocity_steps;
		this->position_count = position_steps.Mult();
		this->velocity_count = velocity_steps.Mult();
		this->state_count = position_steps.Mult() * velocity_steps.Mult();
		this->float_count = this->state_count * this->dim;

		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->state_data, this->float_count * sizeof(float)), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->value_data, this->state_count * sizeof(float)), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemset(this->value_data, 0, this->state_count * sizeof(float)), __FILE__, __LINE__);
	}

	__device__ void GenerateData() {
		unsigned long long state_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);

		// ordering: p0, p1, p2, v0, v1, v2
		Vector4 unpack_data = Indexer::InverseFlatIndex4(state_index, this->position_steps.x, this->position_steps.y, this->position_steps.z);
		
		Vector3 position_steps{ unpack_data.x, unpack_data.y, unpack_data.z };
		Vector3 velocity_steps = Indexer::InverseFlatIndex3(unpack_data.w, this->velocity_steps.x, this->velocity_steps.y);

		Vector3 position_start = this->position_lower + position_steps;
		Vector3 velocity_start = this->velocity_lower + velocity_steps;
		Vector3 position = this->position_lower + position_steps;
		Vector3 velocity = this->velocity_lower + velocity_steps;

		unsigned long long base_index = Indexer::FlatIndex2(0, state_index, this->dim);
		this->state_data[base_index + 0] = position.x;
		this->state_data[base_index + 1] = position.y;
		this->state_data[base_index + 2] = position.z;
		this->state_data[base_index + 3] = velocity.x;
		this->state_data[base_index + 4] = velocity.y;
		this->state_data[base_index + 5] = velocity.z;

		Vector3 target_lower{ -1, 0.5, -1 };
		Vector3 target_upper{ 1, 1, 1 };

		bool is_success = false;
		for (int i = 0; i < this->time_steps; i++) {
			position += velocity * Physics::DeltaTime();
			velocity += Physics::Gravity() * Physics::DeltaTime();

			if (position.IsBounded(target_lower, target_upper)) {
				is_success = true;
				break;
			}
		}

		if (is_success) {
			this->value_data[state_index] = 1.0;
		}
		else {
			this->value_data[state_index] = 0.0;
		}
	}

	void Free() {
		CudaError::CheckError((cudaError_enum)cudaFree(this->state_data), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaFree(this->value_data), __FILE__, __LINE__);
	}
};