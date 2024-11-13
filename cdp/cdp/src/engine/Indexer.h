#pragma once

#include "Transform.h"

struct Indexer {
	static __device__ int FlatIndex3(int x, int y, int z, int x_max, int y_max) {
		int index = x + (y * x_max) + (z * x_max * y_max);

		return index;
	}

	static __device__ int FlatIndex4(int x, int y, int z, int w, int x_max, int y_max, int z_max) {
		int index = x + (y * x_max) + (z * x_max * y_max) + (w * x_max * y_max * z_max);

		return index;
	}
	
	static __device__ Vector3 InverseFlatIndex3(int index, int x_max, int y_max) {
		int xy_progress = index % (x_max * y_max);

		Vector3 result{
			xy_progress % x_max,
			floor(xy_progress / x_max),
			floor(index / (x_max * y_max))
		};

		return result;
	}
	
	static __device__ Vector4 InverseFlatIndex4(int index, int x_max, int y_max, int z_max) {
		int xyz_progress = index % (x_max * y_max * z_max);
		int xy_progress = xyz_progress % (x_max * y_max);

		Vector4 result{
			xy_progress % x_max,
			floor(xy_progress / x_max),
			floor(xyz_progress / (x_max * y_max)),
			floor(index / (x_max * y_max * z_max))
		};

		return result;
	}
	
};