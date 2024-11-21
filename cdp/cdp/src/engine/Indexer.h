#pragma once

#include "Transform.h"

struct Indexer {
	static __device__ unsigned long long FlatIndex2(unsigned long long x, unsigned long long y, unsigned long long x_max) {
		unsigned long long index = x + (y * x_max);

		return index;
	}

	static __device__ unsigned long long FlatIndex3(unsigned long long x, unsigned long long y, unsigned long long z, unsigned long long x_max, unsigned long long y_max) {
		unsigned long long index = x + (y * x_max) + (z * x_max * y_max);

		return index;
	}

	static __device__ unsigned long long FlatIndex4(unsigned long long x, unsigned long long y, unsigned long long z, unsigned long long w, unsigned long long x_max, unsigned long long y_max, unsigned long long z_max) {
		unsigned long long index = x + (y * x_max) + (z * x_max * y_max) + (w * x_max * y_max * z_max);

		return index;
	}
	
	static __device__ Vector3 InverseFlatIndex3(unsigned long long index, unsigned long long x_max, unsigned long long y_max) {
		unsigned long long xy_progress = index % (x_max * y_max);

		Vector3 result{
			xy_progress % x_max,
			floor(xy_progress / x_max),
			floor(index / (x_max * y_max))
		};

		return result;
	}
	
	static __device__ Vector4 InverseFlatIndex4(unsigned long long index, unsigned long long x_max, unsigned long long y_max, unsigned long long z_max) {
		unsigned long long xyz_progress = index % (x_max * y_max * z_max);
		unsigned long long xy_progress = xyz_progress % (x_max * y_max);

		Vector4 result{
			xy_progress % x_max,
			floor(xy_progress / x_max),
			floor(xyz_progress / (x_max * y_max)),
			floor(index / (x_max * y_max * z_max))
		};

		return result;
	}
	
};