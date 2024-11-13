#pragma once

#include <math.h>
#include "cuda.h"
#include "cudart_platform.h"
#include "device_launch_parameters.h"

struct Vector2 {
	float x = 0;
	float y = 0;
};


struct Vector3 {
	float x = 0;
	float y = 0;
	float z = 0;

	__device__ Vector3& operator+(Vector3 operand) {
		Vector3 result{
			this->x + operand.x,
			this->y + operand.y,
			this->z + operand.z
		};

		return result;
	}

	__device__ Vector3 operator-(Vector3 operand) {
		Vector3 result{
			this->x - operand.x,
			this->y - operand.y,
			this->z - operand.z,
		};

		return result;
	}

	__device__ Vector3 operator*(float operand) {
		Vector3 result{
			this->x * operand,
			this->y * operand,
			this->z * operand,
		};

		return result;
	}

	__device__ Vector3 operator/(float operand) {
		Vector3 result{
			this->x / operand,
			this->y / operand,
			this->z / operand
		};

		return result;
	}

	__device__ Vector3& operator+=(Vector3 operand) {
		this->x += operand.x;
		this->y += operand.y;
		this->z += operand.z;

		return *this;
	}

	__device__ Vector3& operator-=(Vector3 operand) {
		this->x -= operand.x;
		this->y -= operand.y;
		this->z -= operand.z;

		return *this;
	}

	__device__ Vector3& operator-() {
		Vector3 result{
			-this->x,
			-this->y,
			-this->z
		};

		return result;
	}

	__device__ bool& operator==(Vector3 operand) {
		bool result = this->x == operand.x;
		result &= this->y == operand.y;
		result &= this->z == operand.z;

		return result;
	}

	__device__ bool& operator!=(Vector3 operand) {
		bool result = this->x == operand.x;
		result &= this->y == operand.y;
		result &= this->z == operand.z;
		result = !result;

		return result;
	}
};

struct Vector4 {
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;

	__device__ Vector4& operator+(Vector4 operand) {
		Vector4 result{
			result.x = this->x + operand.x,
			result.y = this->y + operand.y,
			result.z = this->z + operand.z,
			result.w = this->w + operand.w
		};

		return result;
	}

	__device__ Vector4 operator-(Vector4 operand) {
		Vector4 result{
			this->x - operand.x,
			this->y - operand.y,
			this->z - operand.z,
			this->w - operand.w
		};

		return result;
	}

	__device__ Vector4 operator*(float operand) {
		Vector4 result{
			this->x * operand,
			this->y * operand,
			this->z * operand,
			this->w * operand
		};

		return result;
	}

	__device__ Vector4 operator/(float operand) {
		Vector4 result{
			result.x = this->x / operand,
			result.y = this->y / operand,
			result.z = this->z / operand,
			result.w = this->w / operand
		};

		return result;
	}

	__device__ Vector4& operator+=(Vector4 operand) {
		this->x += operand.x;
		this->y += operand.y;
		this->z += operand.z;
		this->w += operand.w;

		return *this;
	}

	__device__ Vector4& operator-=(Vector4 operand) {
		this->x -= operand.x;
		this->y -= operand.y;
		this->z -= operand.z;
		this->w -= operand.w;

		return *this;
	}

	__device__ Vector4& operator-() {
		Vector4 result{
			-this->x,
			-this->y,
			-this->z,
			-this->w
		};

		return result;
	}
};

struct Transform {
	Vector3 position{ 0, 0, 0 };
	Vector4 rotation{ 0, 0, 0, 1 };

	static __device__ int Clip(int value, int lower, int upper) {
		if (value < lower) {
			value = lower;
		}

		if (value > upper) {
			value = upper;
		}

		return value;
	}

	static __device__ float Norm3(Vector3 vector) {
		float norm_val = (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z);
		norm_val = sqrt(norm_val);

		return norm_val;
	}

	static __device__ float Norm4(Vector4 vector) {
		float norm_val = (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) + (vector.w * vector.w);
		norm_val = sqrt(norm_val);

		return norm_val;
	}

	static __device__ Vector3 Unit3(Vector3 vector) {
		Vector3 unit_vector{};
		float norm_val = Transform::Norm3(vector);
		
		if (norm_val != 0) {
			unit_vector = vector / norm_val;
		}

		return unit_vector;
	}

	static __device__ Vector4 Unit4(Vector4 vector) {
		Vector4 unit_vector{};
		float norm_val = Transform::Norm4(vector);

		if (norm_val != 0) {
			unit_vector = vector / norm_val;
		}

		return unit_vector;
	}
};