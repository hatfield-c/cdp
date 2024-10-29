#include "Camera.h"

Camera::Camera(std::string name, CUdeviceptr gpu_texture) {
	this->name = name;
	this->camera_data.transform.position = Transform::Vector3{ 440, 500, 80 };
	this->camera_data.transform.rotation = this->QuaternionFromEulerAngles(Transform::Vector3{ 0, 0, 0 });
	this->camera_data.fov.x = (float)std::numbers::pi / 2;
	this->camera_data.fov.y = (float)std::numbers::pi / 2;
	this->camera_data.gpu_texture = (byte*)gpu_texture;
}

void Camera::Render(WorldSpace* world_space) {
	RenderCamera(this->camera_data, world_space);
}

Transform::Vector4 Camera::QuaternionFromEulerAngles(Transform::Vector3 angles) {
	float alpha = angles.x / 2;
	float beta = angles.y / 2;
	float gamma = angles.z / 2;

	Transform::Vector4 quaternion{
		(sin(alpha) * cos(beta) * cos(gamma)) - (cos(alpha) * sin(beta) * sin(gamma)),
		(cos(alpha) * sin(beta) * cos(gamma)) + (sin(alpha) * cos(beta) * sin(gamma)),
		(cos(alpha) * cos(beta) * sin(gamma)) - (sin(alpha) * sin(beta) * cos(gamma)),
		(cos(alpha) * cos(beta) * cos(gamma)) + (sin(alpha) * sin(beta) * sin(gamma))
	};

	return quaternion;
}