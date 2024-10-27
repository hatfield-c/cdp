#include "Camera.h"
#include <iostream>

Camera::Camera(std::string name, CUdeviceptr gpu_texture) {
	this->name = name;
	this->camera_data.transform.position = Transform::Vector3{ 440, 500, 80 };
	this->camera_data.transform.rotation = Transform::Vector4{ 0, 0, 0, 1 };
	this->camera_data.fov.x = (float)std::numbers::pi / 2;
	this->camera_data.fov.y = (float)std::numbers::pi / 2;
	this->camera_data.gpu_texture = (byte*)gpu_texture;
}

void Camera::Render(WorldSpace* world_space) {
	RenderCamera(this->camera_data, world_space);
}