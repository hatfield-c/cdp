#include "Transform.h"

Vector4 QuaternionFromEulerAngles(Vector3 angles) {
	float alpha = angles.x / 2;
	float beta = angles.y / 2;
	float gamma = angles.z / 2;

	Vector4 quaternion{
		(sin(alpha) * cos(beta) * cos(gamma)) - (cos(alpha) * sin(beta) * sin(gamma)),
		(cos(alpha) * sin(beta) * cos(gamma)) + (sin(alpha) * cos(beta) * sin(gamma)),
		(cos(alpha) * cos(beta) * sin(gamma)) - (sin(alpha) * sin(beta) * cos(gamma)),
		(cos(alpha) * cos(beta) * cos(gamma)) + (sin(alpha) * sin(beta) * sin(gamma))
	};

	return quaternion;
}