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
	unsigned long long ihm_index = 0;//Indexer::FlatIndex4(0, 48, 4, 42, 24, 100, 30);
	Vector3 camera_position{ 48, 4, 42 };
	int camera_rotation_index = 12;
	int camera_index = 0;
	int control_index = 0;
	bool is_verify_ihm = false;
	bool is_estimate_position = false;
	bool is_save_confusion = false;
	bool is_stochastic_subtraction = false;
	bool is_render_path_confusion = false;
};