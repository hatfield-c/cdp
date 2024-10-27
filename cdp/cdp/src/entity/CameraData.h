#pragma once

#include <string>
#include <numbers>

#include "../engine/Transform.h"

typedef unsigned char byte;

struct CameraData {
	Transform::Transform transform{};
	Transform::Vector2 resolution{ 640, 480 };
	Transform::Vector2 fov{ 1.4, 1.4 };
	Transform::Vector3 target_offset{ 0, -15, 7 };

	float min_render_distance = 0.05f;
	float max_render_distance = 350.0f;

	byte* gpu_texture;
};