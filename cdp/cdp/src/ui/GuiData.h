#pragma once

#include <string>
#include "../engine/Transform.h"

struct GuiData {
	bool is_window_open = false;
	std::string save_phash_path = "";
	std::string load_env_path = "";

	// Simulation
	bool is_simulating = false;
	bool is_paused = false;
	bool is_step_simulation = false;
	bool is_save_simulation_image = false;

	// Camera
	unsigned long long ihm_index = 0;//Indexer::FlatIndex4(0, 48, 4, 42, 24, 100, 30);
	Vector3 camera_position{ 48, 4, 42 };
	int camera_rotation_index = 12;
	int camera_index = 0;
	int control_index = 0;
	bool is_rotation_noise = false;

	// Drone
	Vector3 drone_voxel{};
	Vector3 drone_position{};
	Vector3 drone_forward{};
	Vector4 drone_quaternion{};
	Vector3 drone_velocity{};
	Vector3 drone_angular_velocity{};

	Vector3 keyboard{};

	// Wallride
	Vector3 wallride_sensor;

	// Buttons
	bool is_playground = false;
	bool is_stochastic_subtraction = false;
	bool is_generate_hitpoly_data = false;
	bool is_generate_polyfield_data = false;
};