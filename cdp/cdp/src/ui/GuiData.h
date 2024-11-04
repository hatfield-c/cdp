#pragma once

#include <string>

struct GuiData {
	bool is_window_open = false;
	bool is_simulating = false;
	std::string load_path = "";
	std::string save_path = "";
};