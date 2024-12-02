#pragma once

#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"

typedef unsigned char byte;

struct IhmCortex {
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

				if (state_index == 2508) {
					printf("(%d, %d): %d [%d - %d]\n", j, i, phash_index, phash[phash_index], this->ihm[ihm_index]);
				}

				if (phash[phash_index] - this->ihm[ihm_index] != 0) {
					if (difference_count == 255) {
						continue;
					}

					difference_count++;
				}
			}
		}

		difference_vector[state_index] = difference_count;

		if (state_index == 2508) {
			printf("Diff: %d %d\n", difference_count, difference_vector[state_index]);
		}
	}
};