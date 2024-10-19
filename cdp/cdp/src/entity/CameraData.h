#pragma once

#include <string>
#include <numbers>
#include "../engine/Transform.h"

struct CameraData {
	std::string name = "camera";
	Transform::Transform transform{};
	Transform::Vector2 resolution{ 640, 480 };
	Transform::Vector2 fov{ std::numbers::pi / 2, std::numbers::pi / 2 };
	float min_render_distance = 0.05;
	float max_render_distance = 350;

	Transform::Vector3 target_offset{ 0, -15, 7 };
	CUdeviceptr gpu_texture;
};