#pragma once

#include <string>
#include <numbers>

#include "../engine/Transform.h"

typedef unsigned char byte;

struct CameraData {
	Transform transform{};
	Vector2 resolution{ 640, 480 };
	Vector2 fov{ 1.57, 1.29 };
	Vector3 target_offset{ -1, 1, 0 };

	float min_render_distance = 0.05f;
	float max_render_distance = 350.0f;

	byte* gpu_texture;
};