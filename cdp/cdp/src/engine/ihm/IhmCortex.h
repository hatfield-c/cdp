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
	unsigned long long state_count;

	Vector2 phash_size{ 16, 16 };

	void Init(byte* ihm, byte* ihm_cpu, unsigned long long state_count) {
		this->ihm = ihm;
		this->ihm_cpu = ihm_cpu;
		this->state_count = state_count;
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

		difference_count = Transform::Clip(difference_count, 0, 255);
		difference_vector[state_index] = difference_count;
	}

	// iterate through the query cube voxel positions directly (no reduction). check for optimal +/- offset. store the ihm index for each voxel position
	//		that passes the phash_error_threshold (0 is stored otherwise) (only need to store into smaller offset matrix). manually search these resulting small lists 
	//		to get the average positions from the IHM indexes
	//
	//		dont bother with optimal offset for now. too much uncertainty.

	__device__ void FilterLocalOffsets(IhmGenerator ihm_generator, byte* difference_vector, unsigned long long* index_matrix0, unsigned long long* index_matrix1, unsigned long long* index_matrix2, int direction_index, Vector3 anchor) {
		unsigned long long phash_error_threshold0 = 8;
		unsigned long long phash_error_threshold1 = 78;
		unsigned long long phash_error_threshold2 = 154;
		Vector3 search_radius{ 16, 8, 16 };
		Vector3 search_size = search_radius * 2;
		
		unsigned long long voxel_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);
		
		Vector3 voxel_offset = Indexer::InverseFlatIndex3(voxel_index, search_size.x, search_size.y);

		if (voxel_offset.x >= search_size.x || voxel_offset.y >= search_size.y || voxel_offset.z >= search_size.z) {
			return;
		}

		Vector3 query_position = anchor + (voxel_offset - search_radius);

		if (
			query_position.x < 0 ||
			query_position.y < 0 ||
			query_position.z < 0 ||
			query_position.x >= ihm_generator.world_size_strided.x ||
			query_position.y >= ihm_generator.world_size_strided.y ||
			query_position.z >= ihm_generator.world_size_strided.z
		) {
			return;
		}
		
		unsigned long long ihm_state_index = Indexer::FlatIndex4(direction_index, query_position.x, query_position.y, query_position.z, ihm_generator.direction_count, ihm_generator.world_width_strided.x, ihm_generator.world_width_strided.y);
		byte phash_score = difference_vector[ihm_state_index];

		if (phash_score <= phash_error_threshold0) {
			index_matrix0[voxel_index] = ihm_state_index;
		}
		
		if (phash_score <= phash_error_threshold1) {
			index_matrix1[voxel_index] = ihm_state_index;
		}
		
		if (phash_score <= phash_error_threshold2) {
			index_matrix2[voxel_index] = ihm_state_index;
		}

		//if (threadIdx.x == 0 && blockIdx.x == 0) {
			//printf("%lld %d %d { %.2f, %.2f, %.2f } [ %.2f, %.2f, %.2f ] ( %.2f, %.2f, %.2f )\n", voxel_index, threadIdx.x, blockIdx.x, anchor.x, anchor.y, anchor.z, voxel_offset.x, voxel_offset.y, voxel_offset.z, query_position.x, query_position.y, query_position.z);
		//}
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
	
	__device__ void GetSimilarityScore(int iteration, byte difference_threshold, byte* difference_vector, double* score_buffer, void(*SyncThreads)()) {
		unsigned long long thread_0_index = Indexer::FlatIndex3(0, 0, blockIdx.x, this->thread_units, blockDim.x);

		int units_per_block = this->thread_units * blockDim.x;
		unsigned long long value_stride = pow(units_per_block, iteration);

		unsigned long long base_index = Indexer::FlatIndex3(0, threadIdx.x, blockIdx.x, this->thread_units, blockDim.x);
		base_index = base_index * value_stride;

		unsigned long long base_score_index = floor(((double)base_index) / ((double)this->thread_units));

		if (base_index >= this->state_count) {
			return;
		}

		double thread_score = 0;
		unsigned long long unit_index_buffer;
		for (int i = 0; i < this->thread_units; i++) {
			unsigned long long unit_index = Indexer::FlatIndex3(i, threadIdx.x, blockIdx.x, this->thread_units, blockDim.x);
			unit_index = unit_index * value_stride;

			if (unit_index >= this->state_count) {
				break;
			}

			if (iteration == 0) {
				byte difference_value = difference_vector[unit_index];
				
				if (difference_value <= difference_threshold) {
					thread_score++;
				}
			}
			else {
				unit_index_buffer = floor(((double)unit_index) / ((double)this->thread_units));
				thread_score += score_buffer[unit_index_buffer];
			}
		}

		score_buffer[base_score_index] = thread_score;

		SyncThreads();

		if (threadIdx.x > 0) {
			return;
		}
		
		double block_score = 0;
		for (int i = 0; i < blockDim.x; i++) {
			unsigned long long unit_index = Indexer::FlatIndex3(0, i, blockIdx.x, this->thread_units, blockDim.x);

			unit_index = unit_index * value_stride;

			if (unit_index >= this->state_count) {
				break;
			}

			unit_index_buffer = floor(((double)unit_index) / ((double)this->thread_units));
			block_score += score_buffer[unit_index_buffer];
		}
		
		score_buffer[base_score_index] = block_score;
	}
};