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

	__device__ void GetDifferenceVector(byte* phash, byte* difference_vector) {
		unsigned long long state_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);

		byte* ihm = this->ihm_cpu;

		int difference_count = 0;
		for (int i = 0; i < this->phash_size.x; i++) {
			for (int j = 0; j < this->phash_size.y; j++) {
				unsigned long long ihm_index = Indexer::FlatIndex3(j, i, state_index, this->phash_size.x, this->phash_size.y);
				int phash_index = Indexer::FlatIndex2(j, i, this->phash_size.x);

				if (phash[phash_index] - this->ihm[ihm_index] != 0) {
					if (difference_count == 255) {
						continue;
					}

					difference_count++;
				}
			}
		}

		difference_vector[state_index] = difference_count;
	}

	__device__ void IndexReduction(int iteration, byte* difference_vector, unsigned long long* index_buffer, void(*SyncThreads)()) {
		int units_per_block = this->thread_units * blockDim.x;
		unsigned long long value_stride = pow(units_per_block, iteration);
		
		unsigned long long base_index = Indexer::FlatIndex3(0, threadIdx.x, blockIdx.x, this->thread_units, blockDim.x);
		
		base_index = base_index * value_stride;

		//unsigned long long buffer_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);
		unsigned long long base_index_buffer = floor(((double)base_index) / ((double)this->thread_units));

		if (base_index >= this->state_count) {
			return;
		}

		if (base_index == 0) {
			printf("[it]: %d\n", iteration);
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

			if (difference_value == 0) {
				printf("    [Unit]: %lld %d %lld %lld %lld\n", unit_index, lowest_value, lowest_unit, base_index_buffer, unit_index_buffer);
			}
		}

		difference_vector[base_index] = lowest_value;
		index_buffer[base_index_buffer] = lowest_unit;

		if (base_index == 3034624) {
			//printf("    [cuda-b]: %d %d %d\n", threadIdx.x, blockIdx.x, blockDim.x);
		}

		if (lowest_value == 0) {
			printf("    [base1]: %lld %lld %lld | %d %lld\n", base_index, lowest_unit, base_index_buffer, difference_vector[base_index], index_buffer[base_index_buffer]);
			printf("    [cuda]: %d %d %d\n", threadIdx.x, blockIdx.x, blockDim.x);
		}

		SyncThreads();

		if (threadIdx.x > 0) {
			return;
		}

		if (base_index == 0) {
			printf("    [index]: %d %d %d - %lld, %lld, %lld, %lld, %lld\n", difference_vector[3034752], difference_vector[9641], difference_vector[32], index_buffer[11854], index_buffer[11855], index_buffer[32], index_buffer[878], index_buffer[55555]);
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

			if (difference_value == 0) {
				printf("    %d %d %lld\n", difference_value, lowest_value, lowest_unit);
			}
		}

		difference_vector[base_index] = lowest_value;
		index_buffer[base_index_buffer] = lowest_unit;

		if (lowest_value == 0) {
			printf("    [base2]: %lld %lld %lld %lld | %d %lld\n", base_index, lowest_unit, base_index_buffer, unit_index_buffer, difference_vector[base_index], index_buffer[base_index_buffer]);
		}
	}
};