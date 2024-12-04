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

	__device__ void IndexReduction(int iteration, byte* difference_vector, void(*SyncThreads)()) {
		int units_per_block = this->thread_units * blockDim.x;
		unsigned long long stride = pow(units_per_block, iteration);

		byte lowest_value = 255;
		for (int i = 0; i < this->thread_units; i++) {
			
			unsigned long long unit_index = Indexer::FlatIndex3(i, threadIdx.x, blockIdx.x, this->thread_units, blockDim.x);
			unit_index = unit_index * stride;

			if (unit_index >= this->state_count) {
				break;
			}

			byte difference_value = difference_vector[unit_index];

			if (difference_value < lowest_value) {
				lowest_value = difference_value;
			}
		}

		unsigned long long base_index = Indexer::FlatIndex3(0, threadIdx.x, blockIdx.x, this->thread_units, blockDim.x);
		base_index = base_index * stride;

		difference_vector[base_index] = lowest_value;

		SyncThreads();

		if (threadIdx.x > 0) {
			return;
		}

		lowest_value = 255;
		for (int i = 0; i < blockDim.x; i++) {
			unsigned long long unit_index = Indexer::FlatIndex3(0, i, blockIdx.x, this->thread_units, blockDim.x);
			unit_index = unit_index * stride;

			if (unit_index >= this->state_count) {
				break;
			}

			byte difference_value = difference_vector[unit_index];

			if (difference_value < lowest_value) {
				lowest_value = difference_value;
			}
		}

		difference_vector[base_index] = lowest_value;
	}
};