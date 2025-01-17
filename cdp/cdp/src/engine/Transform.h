#pragma once

#include <math.h>
#include "cuda.h"
#include "cudart_platform.h"
#include "device_launch_parameters.h"

struct Vector2 {
	float x = 0;
	float y = 0;

	__host__ __device__ Vector2& operator+(Vector2 operand) {
		Vector2 result{
			this->x + operand.x,
			this->y + operand.y
		};

		return result;
	}

	__host__ __device__ Vector2 operator-(Vector2 operand) {
		Vector2 result{
			this->x - operand.x,
			this->y - operand.y
		};

		return result;
	}

	__host__ __device__ Vector2 operator-(float operand) {
		Vector2 result{
			this->x - operand,
			this->y - operand
		};

		return result;
	}

	__host__ __device__ Vector2 operator*(float operand) {
		Vector2 result{
			this->x * operand,
			this->y * operand
		};

		return result;
	}

	__host__ __device__ Vector2 operator*(Vector2 operand) {
		Vector2 result{
			this->x * operand.x,
			this->y * operand.y
		};

		return result;
	}

	__host__ __device__ Vector2 operator/(float operand) {
		Vector2 result{
			this->x / operand,
			this->y / operand
		};

		return result;
	}

	__host__ __device__ Vector2 operator/(Vector2 operand) {
		Vector2 result{
			this->x / operand.x,
			this->y / operand.y
		};

		return result;
	}

	__host__ __device__ Vector2& operator+=(Vector2 operand) {
		this->x += operand.x;
		this->y += operand.y;

		return *this;
	}

	__host__ __device__ Vector2& operator-=(Vector2 operand) {
		this->x -= operand.x;
		this->y -= operand.y;

		return *this;
	}

	__host__ __device__ Vector2& operator-() {
		Vector2 result{
			-this->x,
			-this->y
		};

		return result;
	}

	__host__ __device__ bool& operator==(Vector2 operand) {
		bool result = this->x == operand.x;
		result &= this->y == operand.y;

		return result;
	}

	__host__ __device__ bool& operator!=(Vector2 operand) {
		bool result = this->x == operand.x;
		result &= this->y == operand.y;
		result = !result;

		return result;
	}
};


struct Vector3 {
	float x = 0;
	float y = 0;
	float z = 0;

	__host__ __device__ Vector3& operator+(Vector3 operand) {
		Vector3 result{
			this->x + operand.x,
			this->y + operand.y,
			this->z + operand.z
		};

		return result;
	}

	__host__ __device__ Vector3 operator-(Vector3 operand) {
		Vector3 result{
			this->x - operand.x,
			this->y - operand.y,
			this->z - operand.z,
		};

		return result;
	}

	__host__ __device__ Vector3 operator-(float operand) {
		Vector3 result{
			this->x - operand,
			this->y - operand,
			this->z - operand,
		};

		return result;
	}

	__host__ __device__ Vector3 operator*(float operand) {
		Vector3 result{
			this->x * operand,
			this->y * operand,
			this->z * operand,
		};

		return result;
	}

	__host__ __device__ Vector3 operator*(Vector3 operand) {
		Vector3 result{
			this->x * operand.x,
			this->y * operand.y,
			this->z * operand.z
		};

		return result;
	}

	__host__ __device__ Vector3 operator/(float operand) {
		Vector3 result{
			this->x / operand,
			this->y / operand,
			this->z / operand
		};

		return result;
	}

	__host__ __device__ Vector3 operator/(Vector3 operand) {
		Vector3 result{
			this->x / operand.x,
			this->y / operand.y,
			this->z / operand.z
		};

		return result;
	}

	__host__ __device__ Vector3& operator+=(Vector3 operand) {
		this->x += operand.x;
		this->y += operand.y;
		this->z += operand.z;

		return *this;
	}

	__host__ __device__ Vector3& operator-=(Vector3 operand) {
		this->x -= operand.x;
		this->y -= operand.y;
		this->z -= operand.z;

		return *this;
	}

	__host__ __device__ Vector3& operator-() {
		Vector3 result{
			-this->x,
			-this->y,
			-this->z
		};

		return result;
	}

	__host__ __device__ bool& operator==(Vector3 operand) {
		bool result = this->x == operand.x;
		result &= this->y == operand.y;
		result &= this->z == operand.z;

		return result;
	}

	__host__ __device__ bool& operator!=(Vector3 operand) {
		bool result = this->x == operand.x;
		result &= this->y == operand.y;
		result &= this->z == operand.z;
		result = !result;

		return result;
	}

	__host__ __device__ Vector3 Floor() {
		Vector3 result{
			floor(this->x),
			floor(this->y),
			floor(this->z)
		};

		return result;
	}

	__host__ __device__ Vector3 Ceil() {
		Vector3 result{
			ceil(this->x),
			ceil(this->y),
			ceil(this->z)
		};

		return result;
	}
};

struct Vector4 {
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;

	__host__ __device__ Vector4& operator+(Vector4 operand) {
		Vector4 result{
			result.x = this->x + operand.x,
			result.y = this->y + operand.y,
			result.z = this->z + operand.z,
			result.w = this->w + operand.w
		};

		return result;
	}

	__host__ __device__ Vector4 operator-(Vector4 operand) {
		Vector4 result{
			this->x - operand.x,
			this->y - operand.y,
			this->z - operand.z,
			this->w - operand.w
		};

		return result;
	}

	__host__ __device__ Vector4 operator*(float operand) {
		Vector4 result{
			this->x * operand,
			this->y * operand,
			this->z * operand,
			this->w * operand
		};

		return result;
	}

	__host__ __device__ Vector4 operator/(float operand) {
		Vector4 result{
			result.x = this->x / operand,
			result.y = this->y / operand,
			result.z = this->z / operand,
			result.w = this->w / operand
		};

		return result;
	}

	__host__ __device__ Vector4& operator+=(Vector4 operand) {
		this->x += operand.x;
		this->y += operand.y;
		this->z += operand.z;
		this->w += operand.w;

		return *this;
	}

	__host__ __device__ Vector4& operator-=(Vector4 operand) {
		this->x -= operand.x;
		this->y -= operand.y;
		this->z -= operand.z;
		this->w -= operand.w;

		return *this;
	}

	__host__ __device__ Vector4& operator-() {
		Vector4 result{
			-this->x,
			-this->y,
			-this->z,
			-this->w
		};

		return result;
	}
};

struct Vector {

	static __host__ __device__ Vector2 ZERO2() {
		return Vector2{ 0, 0 };
	}
	static __host__ __device__ Vector3 ZERO3() {
		return Vector3{ 0, 0, 0 };
	}

	static __host__ __device__ Vector2 ONE2() {
		return Vector2{ 1, 1 };
	}

	static __host__ __device__ Vector3 ONE3() {
		return Vector3{ 1, 1, 1 };
	}

	static __host__ __device__ Vector2 RIGHT2() {
		return Vector2{ 1, 0 };
	}

	static __host__ __device__ Vector2 UP2(){ 
		return Vector2{ 0, 1 };
	}

	static __host__ __device__ Vector2 LEFT2(){ 
		return Vector2{ -1, 0 };
	}

	static __host__ __device__ Vector2 DOWN2() {
		return Vector2{ 0, -1 };
	}

	static __host__ __device__ Vector3 FORWARD() {
		return Vector3{ 0, 0, 1 };
	}

	static __host__ __device__ Vector3 RIGHT() {
		return Vector3{ 1, 0, 0 };
	}

	static __host__ __device__ Vector3 UP() {
		return Vector3{ 0, 1, 0 };
	}

	static __host__ __device__ Vector3 BACKWARD() {
		return Vector3{ 0, 0, -1 };
	}

	static __host__ __device__ Vector3 LEFT() {
		return Vector3{ -1, 0, 0 };
	}

	static __host__ __device__ Vector3 DOWN() {
		return Vector3{ 0, -1, 0 };
	}
};

struct Transform {
	Vector3 position{ 0, 0, 0 };
	Vector4 rotation{ 0, 0, 0, 1 };

	static __host__ __device__ int Clip(int value, int lower, int upper) {
		if (value < lower) {
			value = lower;
		}

		if (value > upper) {
			value = upper;
		}

		return value;
	}

	static __host__ __device__ double Clip(double value, double lower, double upper) {
		if (value < lower) {
			value = lower;
		}

		if (value > upper) {
			value = upper;
		}

		return value;
	}

	static __host__ __device__ float Norm2(Vector2 vector) {
		float norm_val = (vector.x * vector.x) + (vector.y * vector.y);
		norm_val = sqrt(norm_val);

		return norm_val;
	}

	static __host__ __device__ float Norm3(Vector3 vector) {
		float norm_val = (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z);
		norm_val = sqrt(norm_val);

		return norm_val;
	}

	static __host__ __device__ float Norm4(Vector4 vector) {
		float norm_val = (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) + (vector.w * vector.w);
		norm_val = sqrt(norm_val);

		return norm_val;
	}

	static __host__ __device__ Vector2 Unit2(Vector2 vector) {
		Vector2 unit_vector{};
		float norm_val = Transform::Norm2(vector);

		if (norm_val != 0) {
			unit_vector = vector / norm_val;
		}

		return unit_vector;
	}

	static __host__ __device__ Vector3 Unit3(Vector3 vector) {
		Vector3 unit_vector{};
		float norm_val = Transform::Norm3(vector);
		
		if (norm_val != 0) {
			unit_vector = vector / norm_val;
		}

		return unit_vector;
	}

	static __host__ __device__ Vector4 Unit4(Vector4 vector) {
		Vector4 unit_vector{};
		float norm_val = Transform::Norm4(vector);

		if (norm_val != 0) {
			unit_vector = vector / norm_val;
		}

		return unit_vector;
	}
};