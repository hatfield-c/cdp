#pragma once

#include <math.h>

struct Vector2 {
	float x = 0;
	float y = 0;
};

struct Vector3 {
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Vector4 {
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

struct Transform {
	Vector3 position{ 0, 0, 0 };
	Vector4 rotation{ 0, 0, 0, 1 };
};

Vector4 QuaternionFromEulerAngles(Vector3 angles);