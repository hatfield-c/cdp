#pragma once

struct Indexer {
	static __device__ int FlatIndex3(int x, int y, int z, int x_max, int y_max, int z_max) {
		int index = x + (y * x_max) + (z * x_max * y_max);

		return index;
	}
};