#pragma once

//#ifndef __CUDACC__  
//#define __CUDACC__
//#endif

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"

typedef unsigned char byte;

struct IhmCortex {
	int thread_units = 256;
	byte* ihm;
	byte* ihm_cpu;
	Vector3* ihm_clouds;
	unsigned long long state_count;

	Vector2 phash_size{ 16, 16 };
	int phash_count;

	void Init(byte* ihm, byte* ihm_cpu, Vector3* ihm_clouds, unsigned long long state_count) {
		this->ihm = ihm;
		this->ihm_cpu = ihm_cpu;
		this->ihm_clouds = ihm_clouds;
		this->state_count = state_count;
		this->phash_count = this->phash_size.x * this->phash_size.y;
	}

	__device__ void GetDifferenceVector(byte* phash, byte* difference_vector, unsigned long long difference_threshold = 0) {
		unsigned long long state_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);

		byte* ihm = this->ihm_cpu;

		unsigned long long difference_count = 0;
		for (int i = 0; i < this->phash_size.x; i++) {
			for (int j = 0; j < this->phash_size.y; j++) {
				unsigned long long ihm_index = Indexer::FlatIndex3(j, i, state_index, this->phash_size.x, this->phash_size.y);
				int phash_index = Indexer::FlatIndex2(j, i, this->phash_size.x);

				int phash_value = phash[phash_index];
				int ihm_value = this->ihm[ihm_index];
				int value_diff = phash_value - ihm_value;
				value_diff = abs(value_diff);

				if (value_diff > difference_threshold) {
					difference_count++;
				}
			}
		}

		difference_count = Math::Clip(difference_count, 0, 255);
		difference_vector[state_index] = difference_count;
	}

	__device__ void GetChamferDistances(IhmGenerator ihm_generator, Vector3* cloud, float* distances, float* buffer, Vector3 lower, Vector3 upper, unsigned long long thread_units, void(*SyncThreads)()) {
		unsigned long long state_index = blockIdx.x;

		if (state_index >= ihm_generator.state_count) {
			return;
		}

		Vector3 width = upper - lower;
		Vector4 state_data = Indexer::InverseFlatIndex4(state_index, ihm_generator.direction_count, width.x, width.y);
		int direction_index = state_data.x;
		Vector3 local_position{ state_data.y, state_data.z, state_data.w };

		Vector3 voxel_position = lower + local_position;
		unsigned long long ihm_state_index = Indexer::FlatIndex4(direction_index, voxel_position.x, voxel_position.y, voxel_position.z, ihm_generator.direction_count, ihm_generator.world_size_strided.x, ihm_generator.world_width_strided.y);

		if (!voxel_position.IsBounded(Vector::ZERO3(), ihm_generator.world_width_strided - 1)) {
			return;
		}

		float distance = 0;
		for (int i = 0; i < thread_units; i++) {
			unsigned long long index_a0 = Indexer::FlatIndex2(i, threadIdx.x, thread_units);
			unsigned long long index_b0 = index_a0 + Indexer::FlatIndex3(0, 0, ihm_state_index, this->phash_size.x, this->phash_size.y);

			Vector3 a0 = cloud[index_a0];
			Vector3 b0 = this->ihm_clouds[index_b0];

			float shortest_a = 1000000000000;
			float shortest_b = 1000000000000;
			for (int j = 0; j < ihm_generator.phash_count; j++) {
				unsigned long long index_a1 = j + Indexer::FlatIndex3(0, 0, ihm_state_index, this->phash_size.x, this->phash_size.y);
				unsigned long long index_b1 = j;

				Vector3 a1 = this->ihm_clouds[index_a1];
				Vector3 b1 = cloud[index_b1];

				float a_distance = Transform::Norm3(a1 - a0);
				float b_distance = Transform::Norm3(b1 - b0);

				if (a_distance < shortest_a) {
					shortest_a = a_distance;
				}

				if (b_distance < shortest_b) {
					shortest_b = b_distance;
				}
			}

			distance += shortest_a + shortest_b;
		}

		unsigned long long thread_buffer_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);
		buffer[thread_buffer_index] = distance;

		SyncThreads();

		if (threadIdx.x > 0) {
			return;
		}

		float chamfer_distance = 0;
		for (int i = 0; i < blockDim.x; i++) {
			unsigned long long buffer_index = Indexer::FlatIndex2(i, blockIdx.x, blockDim.x);
			chamfer_distance += buffer[buffer_index];
		}

		distances[ihm_state_index] = chamfer_distance / (2 * this->phash_count);

		if (ihm_state_index == 1018764) {
			printf("%.2f <%lld %lld> [%.2f %.2f %.2f] {%.2f %.2f %.2f}\n", distances[ihm_state_index], state_index, ihm_state_index, local_position.x, local_position.y, local_position.z, voxel_position.x, voxel_position.y, voxel_position.z);
		}
	}

	__device__ void IndexReduction(int iteration, byte* difference_vector, unsigned long long* index_buffer, void(*SyncThreads)()) {
		int units_per_block = this->thread_units * blockDim.x;
		unsigned long long value_stride = pow(units_per_block, iteration);
		
		unsigned long long base_index = Indexer::FlatIndex3(0, threadIdx.x, blockIdx.x, this->thread_units, blockDim.x);
		base_index = base_index * value_stride;

		unsigned long long base_index_buffer = floor(((double)base_index) / ((double)this->thread_units));

		if (base_index >= this->state_count) {
			return;
		}

		byte lowest_value = 255;
		unsigned long long lowest_unit;
		unsigned long long unit_index_buffer;
		for (int i = 0; i < this->thread_units; i++) {
			unsigned long long unit_index = Indexer::FlatIndex3(i, threadIdx.x, blockIdx.x, this->thread_units, blockDim.x);
			unit_index = unit_index * value_stride;

			if (unit_index >= this->state_count) {
				break;
			}

			byte difference_value = difference_vector[unit_index];

			if (difference_value <= lowest_value) {
				lowest_value = difference_value;

				if (iteration == 0) {
					lowest_unit = unit_index;
				}
				else {
					unit_index_buffer = floor(((double)unit_index) / ((double)this->thread_units));
					lowest_unit = index_buffer[unit_index_buffer];
				}
			}
		}

		difference_vector[base_index] = lowest_value;
		index_buffer[base_index_buffer] = lowest_unit;

		SyncThreads();

		if (threadIdx.x > 0) {
			return;
		}

		lowest_value = 255;
		lowest_unit = 0;
		for (int i = 0; i < blockDim.x; i++) {
			unsigned long long unit_index = Indexer::FlatIndex3(0, i, blockIdx.x, this->thread_units, blockDim.x);

			unit_index = unit_index * value_stride;

			if (unit_index >= this->state_count) {
				break;
			}

			byte difference_value = difference_vector[unit_index];

			if (difference_value < lowest_value) {
				lowest_value = difference_value;

				unit_index_buffer = floor(((double)unit_index) / ((double)this->thread_units));
				lowest_unit = index_buffer[unit_index_buffer];
			}
		}

		difference_vector[base_index] = lowest_value;
		index_buffer[base_index_buffer] = lowest_unit;
	}
	
};