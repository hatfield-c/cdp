#include "Camera.h"

Camera::Camera(std::string name, CUdeviceptr gpu_texture) {
	this->camera_data.name = name;
	this->camera_data.transform.position = Transform::Vector3{ 500, 440, 80 };
	this->camera_data.transform.rotation = Transform::Vector4{ 0, 0, 0, 1 };
	this->camera_data.gpu_texture = gpu_texture;
}

void Camera::Render(WorldSpace* world_space) {

}