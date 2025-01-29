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
	
};