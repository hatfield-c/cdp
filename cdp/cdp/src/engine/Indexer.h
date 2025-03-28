#pragma once

#include "Transform.h"

struct Indexer {
	static __host__ __device__ unsigned long long FlatIndex2(unsigned long long x, unsigned long long y, unsigned long long x_max) {
		unsigned long long index = x + (y * x_max);

		return index;
	}

	static __host__ __device__ unsigned long long FlatIndex3(unsigned long long x, unsigned long long y, unsigned long long z, unsigned long long x_max, unsigned long long y_max) {
		unsigned long long index = x + (y * x_max) + (z * x_max * y_max);

		return index;
	}

	static __host__ __device__ unsigned long long FlatIndex4(unsigned long long x, unsigned long long y, unsigned long long z, unsigned long long w, unsigned long long x_max, unsigned long long y_max, unsigned long long z_max) {
		unsigned long long index = x + (y * x_max) + (z * x_max * y_max) + (w * x_max * y_max * z_max);

		return index;
	}

	static __host__ __device__ unsigned long long FlatIndex2(float x, float y, float x_max) {
		return FlatIndex2((unsigned long long)x, (unsigned long long)y, (unsigned long long)x_max);
	}

	static __host__ __device__ unsigned long long FlatIndex3(float x, float y, float z, float x_max, float y_max) {
		return FlatIndex3((unsigned long long)x, (unsigned long long)y, (unsigned long long)z, (unsigned long long)x_max, (unsigned long long)y_max);
	}

	static __host__ __device__ unsigned long long FlatIndex4(float x, float y, float z, float w, float x_max, float y_max, float z_max) {
		return FlatIndex4((unsigned long long)x, (unsigned long long)y, (unsigned long long)z, (unsigned long long)w, (unsigned long long)x_max, (unsigned long long)y_max, (unsigned long long)z_max);
	}
	
	static __host__ __device__ Vector2 InverseFlatIndex2(unsigned long long index, unsigned long long x_max) {
		unsigned long long x_progress = index % x_max;

		Vector2 result{
			(float)x_progress,
			(float)floor((double)index / (double)x_max),
		};

		return result;
	}

	static __host__ __device__ Vector3 InverseFlatIndex3(unsigned long long index, unsigned long long x_max, unsigned long long y_max) {
		unsigned long long xy_progress = index % (x_max * y_max);

		Vector3 result{
			(float)(xy_progress % x_max),
			(float)floor((double)xy_progress / (double)x_max),
			(float)floor((double)index / (double)(x_max * y_max))
		};

		return result;
	}
	
	static __host__ __device__ Vector4 InverseFlatIndex4(unsigned long long index, unsigned long long x_max, unsigned long long y_max, unsigned long long z_max) {
		unsigned long long xyz_progress = index % (x_max * y_max * z_max);
		unsigned long long xy_progress = xyz_progress % (x_max * y_max);

		Vector4 result{
			(float)(xy_progress % x_max),
			(float)floor((double)xy_progress / (double)x_max),
			(float)floor((double)xyz_progress / (double)(x_max * y_max)),
			(float)floor((double)index / (double)(x_max * y_max * z_max))
		};

		return result;
	}
	
	static __host__ __device__ Vector2 InverseFlatIndex2(float index, float x_max) {
		return InverseFlatIndex2((unsigned long long)index, (unsigned long long)x_max);
	}

	static __host__ __device__ Vector3 InverseFlatIndex3(float index, float x_max, float y_max) {
		return InverseFlatIndex3((unsigned long long)index, (unsigned long long)x_max, (unsigned long long)y_max);
	}

	static __host__ __device__ Vector4 InverseFlatIndex4(float index, float x_max, float y_max, float z_max) {
		return InverseFlatIndex4((unsigned long long)index, (unsigned long long)x_max, (unsigned long long)y_max, (unsigned long long)z_max);
	}

};