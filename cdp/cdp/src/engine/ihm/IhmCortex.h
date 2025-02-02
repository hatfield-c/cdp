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
	
	float* pixel_distances;
	float* chamfer_distances;

	Vector3 search_radius{ 5, 5, 5 };
	Vector3 search_width{ 11, 11, 11 };
	Vector2 phash_size{ 16, 16 };
	int direction_count;
	unsigned long long phash_count;
	unsigned long long state_count;
	unsigned long long pixel_count;

	unsigned long long chamfer_memory_size;
	unsigned long long distance_memory_size;

	void Init(byte* ihm, byte* ihm_cpu, Vector3* ihm_clouds, int direction_count) {
		this->ihm = ihm;
		this->ihm_cpu = ihm_cpu;
		this->ihm_clouds = ihm_clouds;
		this->direction_count = direction_count;
		this->state_count = this->direction_count * this->search_width.Mult();
		this->phash_count = this->phash_size.x * this->phash_size.y;
		this->pixel_count = this->phash_count * this->state_count;

		this->pixel_distances;
		this->distance_memory_size = pixel_count * sizeof(float);
		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->pixel_distances, this->distance_memory_size), __FILE__, __LINE__);

		this->chamfer_distances;
		this->chamfer_memory_size = state_count * sizeof(float);
		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->chamfer_distances, this->chamfer_memory_size), __FILE__, __LINE__);
	}

	void ResetDistanceBuffers() {
		cudaMemset(this->pixel_distances, 0, this->distance_memory_size);
		cudaMemset(this->chamfer_distances, 0, this->chamfer_memory_size);
		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
	}

	__device__ void UpdateNearestDistances(IhmGenerator ihm_generator, Vector3* camera_cloud, Vector3 anchor) {
		unsigned long long region_pixel_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);
		
		Vector3 index_data = Indexer::InverseFlatIndex3(region_pixel_index, this->phash_size.x, this->phash_size.y);
		Vector2 pixel_position{ index_data.x, index_data.y };
		unsigned long long region_state_index = index_data.z;
		
		Vector4 region_state_data = Indexer::InverseFlatIndex4(region_state_index, ihm_generator.direction_count, this->search_width.x, this->search_width.y);
		int direction_index = region_state_data.x;
		Vector3 local_position{ region_state_data.y, region_state_data.z, region_state_data.w };
		Vector3 voxel_position = anchor - this->search_radius + local_position;

		unsigned long long ihm_state_index = Indexer::FlatIndex4(direction_index, voxel_position.x, voxel_position.y, voxel_position.z, ihm_generator.direction_count, ihm_generator.world_width_strided.x, ihm_generator.world_width_strided.y);

		//if (local_position == Vector3{ 5, 5, 5 }) {
		//if (local_position.y == 5) {
			//printf("%.2f <%lld %lld> %d [%.2f %.2f %.2f] {%.2f %.2f %.2f}\n", this->pixel_distances[ihm_state_index], region_state_index, ihm_state_index, direction_index, local_position.x, local_position.y, local_position.z, voxel_position.x, voxel_position.y, voxel_position.z);
		//}

		if (!voxel_position.IsBounded(Vector::ZERO3(), ihm_generator.world_width_strided - 1)) {
			return;
		}

		unsigned long long index_a0 = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, this->phash_size.x);
		unsigned long long index_b0 = Indexer::FlatIndex3(pixel_position.x, pixel_position.y, ihm_state_index, this->phash_size.x, this->phash_size.y);

		Vector3 a0 = camera_cloud[index_a0];
		Vector3 b0 = this->ihm_clouds[index_b0];

		float shortest_a = 1000000000000;
		float shortest_b = 1000000000000;
		for (int j = 0; j < ihm_generator.phash_count; j++) {
			unsigned long long index_a1 = j + Indexer::FlatIndex3(0, 0, ihm_state_index, this->phash_size.x, this->phash_size.y);
			unsigned long long index_b1 = j;

			Vector3 a1 = this->ihm_clouds[index_a1];
			Vector3 b1 = camera_cloud[index_b1];

			float a_distance = Transform::Norm3(a1 - a0);
			float b_distance = Transform::Norm3(b1 - b0);

			if (a_distance < shortest_a) {
				shortest_a = a_distance;
			}

			if (b_distance < shortest_b) {
				shortest_b = b_distance;
			}
		}

		this->pixel_distances[region_pixel_index] = shortest_a + shortest_b;

		//if (ihm_state_index == 1018764 || region_state_index == 15972) {
		//if (local_position == Vector3{ 5, 5, 5 }){
			//printf("%.2f <%lld %lld> [%.2f %.2f %.2f] {%.2f %.2f %.2f}\n", this->pixel_distances[ihm_state_index], region_state_index, ihm_state_index, local_position.x, local_position.y, local_position.z, voxel_position.x, voxel_position.y, voxel_position.z);
		//}
	}

	__device__ void UpdateChamferDistances(IhmGenerator ihm_generator) {
		unsigned long long region_state_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);
		
		if (region_state_index >= this->state_count) {
			return;
		}

		float chamfer_distance = 0;
		for (int i = 0; i < this->phash_count; i++) {
			unsigned long long pixel_index = i + Indexer::FlatIndex3(0, 0, region_state_index, this->phash_size.x, this->phash_size.y);

			chamfer_distance += this->pixel_distances[pixel_index];
		}

		this->chamfer_distances[region_state_index] = chamfer_distance / (2 * this->phash_count);

		Vector4 index_data = Indexer::InverseFlatIndex4(region_state_index, this->direction_count, this->search_width.x, this->search_width.y);
		int direction_index = index_data.x;
		Vector3 local{ index_data.y, index_data.z, index_data.w };

		//if (local == Vector3{ 5, 5, 5 }){//&& index_data.x == 12) {
			//printf("%.2f <%lld> [%.2f %.2f %.2f] %d\n", this->chamfer_distances[region_state_index], region_state_index, local.x, local.y, local.z, direction_index);
		//}
	}
	/*
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
	*/
};