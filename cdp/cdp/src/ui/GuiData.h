#pragma once

#include <string>
#include "../engine/Transform.h"

struct GuiData {
	bool is_window_open = false;
	bool is_simulating = false;
	std::string load_path = "";
	std::string save_path = "";
	Vector3 render_position{};
	Vector3 render_rotation{};
	unsigned long long ihm_position_index = 0;
	int camera_index = 0;
	int vote_threshold = 0;
};