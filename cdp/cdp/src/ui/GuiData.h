#pragma once

#include <string>
#include "../engine/Transform.h"

struct GuiData {
	bool is_window_open = false;
	bool is_simulating = false;
	std::string load_env_path = "";
	std::string save_env_path = "";
	std::string load_ihm_path = "";
	std::string save_ihm_path = "";
	unsigned long long ihm_position_index = 0;
	int camera_index = 0;
	int vote_threshold = 0;
	bool is_verify_ihm = false;
	bool is_save_heatmap = false;
};