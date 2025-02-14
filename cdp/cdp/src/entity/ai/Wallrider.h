#pragma once

#include "../../engine/Transform.h"
#include "../../engine/Math.h"
#include "PlanStep.h"

struct Wallrider {
	// stages are 0:wallride, 1:transit
	int current_stage = 0;
	int plan_index = 0;
	int plan_size = 1;
	PlanStep plan[1] = {
		PlanStep{ -1, "", 7 }
	};

	Vector2 phash_size{ 16, 16 };
	int phash_count = 16 * 16;
	float max_distance = 20.0;

	float* height_phash = new float[16 * 16];
	float* proximity_phash = new float[16];
	byte* height_texture;

	bool brake_proximity;
	bool* steer_proximity = new bool[3];

	void Init() {
		for (int i = 0; i < this->plan_size; i++) {
			PlanStep plan_step = this->plan[i];
			plan_step.Init();
		}
	}

	void Update(float* depth_phash, Vector3* camera_cloud) {
		this->ExtractHeightPhash(depth_phash, camera_cloud);
		this->brake_proximity = false;
		this->steer_proximity[0] = false;
		this->steer_proximity[1] = false;
		this->steer_proximity[2] = false;

		int brake_count = 0;
		int* steer_count = new int[3];
		memset(steer_count, 0, 3 * sizeof(int));

		for (int i = 0; i < 16; i++) {
			float distance = this->proximity_phash[i];

			if (distance < 1) {
				brake_count++;
			}

			if (i > 0 && i < 6) {
				if (distance < 3) {
					steer_count[0]++;
				}
			}
			else if (i > 6 && i < 10) {
				if (distance < 4) {
					steer_count[1]++;
				}
			}
			else if (i > 10 && i < 16) {
				if (distance < 3) {
					steer_count[2]++;
				}
			}
		}

		if (brake_count > 3) {
			this->brake_proximity = true;
		}

		if (steer_count[0] > 2) {
			this->steer_proximity[0] = true;
		}
		
		if (steer_count[1] > 2) {
			this->steer_proximity[1] = true;
		}

		if (steer_count[2] > 2) {
			this->steer_proximity[2] = true;
		}
	}

	Vector3 GetCommand() {
		Vector3 command{ 0, 0, 0 };

		if (this->plan_index >= this->plan_size) {
			return command;
		}

		if (this->IsSafe()) {
			if (this->current_stage == 0) {
				command = this->DoWallride();
			}
			else if (this->current_stage == 1) {
				command = this->DoTransit();
			}
		}

		return command;
	}

	bool IsSafe() {
		if (this->plan_index >= this->plan_size) {
			return false;
		}

		if (this->brake_proximity) {
			return false;
		}

		return true;
	}

	Vector3 DoWallride() {
		Vector3 command{ 0, 0, 1 };
		
		PlanStep plan_step = this->plan[this->plan_index];

		bool is_proximity = this->steer_proximity[0] || this->steer_proximity[1];
		if (plan_step.wall_direction > 0) {
			is_proximity = this->steer_proximity[1] || this->steer_proximity[2];
		}

		command[0] = plan_step.wall_direction;
		if (is_proximity) {
			command[0] = -plan_step.wall_direction;
		}

		/*if (plan_step.IsStartValid()) {
			Vector3 search_result = this->FindSubImage(depth_phash, plan_step.start_phash_cpu, plan_step.target_center, Vector2{ 4, 4 });
			float difference_score = search_result.z;
			printf("%.6f\n", difference_score);
			if (difference_score < 0.0015) {
				this->current_stage++;
				this->transit_kernel = depth_phash;
				this->transit_center = Vector2{ search_result.x, search_result.y };
				this->start_distance_signal = this->VerticalEdgeAverage(depth_phash, this->transit_center);
			}
		}*/
		
		return command;
	}

	Vector3 DoTransit() {
		Vector3 command{ 0, 0, 1 };
		/*
		PlanStep plan_step = this->plan[this->plan_index];
		Vector2 kernel_radius{ 4, 4 };

		Vector3 search_result = this->FindSubImage(depth_phash, this->transit_kernel, this->transit_center, kernel_radius);

		float forward_target = 4;
		float forward_margin = 0.1;

		this->transit_center = Vector2{ search_result.x, search_result.y };
		float steer_error = search_result.x - 7;

		if (abs(steer_error) < 2) {
			this->transit_kernel = depth_phash;
		}

		float distance_signal = this->VerticalEdgeAverage(depth_phash, this->transit_center);
		float signal_delta = abs(this->start_distance_signal - distance_signal);
		float signal_threshold = 2;

		if (signal_delta > signal_threshold && this->forward_distance < forward_target){
			this->current_stage = 0;
			this->plan_index++;

			printf("Plan Step: %d\n", this->plan_index);

			return command;
		}

		if (this->transit_center.x < 7) {
			command.x = -1;
		}
		else if (this->transit_center.x > 7) {
			command.x = 1;
		}
		*/
		return command;
	}

	Vector3 FindSubImage(float* camera_phash, float* condition_phash, Vector2 condition_center, Vector2 kernel_radius) {
		Vector3 sub_center{ 7, 7, 0 };
		Vector2 origin = kernel_radius;
		Vector2 endgin{ this->phash_size.x - kernel_radius.x - 1, this->phash_size.y - kernel_radius.y - 1 };

		float lowest_score = 999999999999999;
		for (int j = origin.y; j < endgin.y; j++) {
			for (int i = origin.x; i < endgin.x; i++) {
				Vector2 camera_center{ i, j };
				float difference_score = this->GetDifferenceScore(camera_phash, condition_phash, camera_center, condition_center, kernel_radius);

				if (difference_score < lowest_score) {
					lowest_score = difference_score;
					sub_center.x = camera_center.x;
					sub_center.y = camera_center.y;
					sub_center.z = difference_score;
				}
			}
		}

		return sub_center;
	}



	float GetDifferenceScore(float* camera_phash, float* condition_phash, Vector2 camera_center, Vector2 condition_center, Vector2 kernel_radius) {
		float score = 0;

		for (int j = -kernel_radius.y; j < kernel_radius.y; j++) {
			for (int i = -kernel_radius.x; i < kernel_radius.x; i++) {
				Vector2 region_position{ i, j };

				Vector2 camera_position = camera_center + region_position;
				Vector2 condition_position = condition_center + region_position;

				if (!camera_position.IsBounded(Vector::ZERO2(), this->phash_size - 1)) {
					continue;
				}

				if (!condition_position.IsBounded(Vector::ZERO2(), this->phash_size - 1)) {
					continue;
				}

				unsigned long long camera_index = Indexer::FlatIndex2(camera_position.x, camera_position.y, this->phash_size.x);
				unsigned long long condition_index = Indexer::FlatIndex2(condition_position.x, condition_position.y, this->phash_size.x);

				float camera_depth = camera_phash[camera_index];
				float condition_depth = condition_phash[condition_index];
				float pixel_score = abs(camera_depth - condition_depth) / 20;

				score += pixel_score * pixel_score;
			}
		}

		score = score / (((2 * kernel_radius.x) + 1) * ((2 * kernel_radius.y) + 1));

		return score;
	}

	void ExtractHeightPhash(float* depth_phash, Vector3* camera_cloud) {
		float collision_bounds = -2;

		for (int i = 0; i < 256; i++) {
			this->height_phash[i] = -1000;
		}

		for (int i = 0; i < 16; i++) {
			this->proximity_phash[i] = 16;
		}

		for (int i = 0; i < 256; i++) {
			Vector3 cloud_position = camera_cloud[i];

			Vector2 phash_position = Indexer::InverseFlatIndex2(i, 16);

			Vector2 height_position{
				phash_position.x,
				round((cloud_position.x / this->max_distance) * (this->phash_size.y - 1))
			};

			if (depth_phash[i] < this->max_distance) {
				unsigned long long index = Indexer::FlatIndex2(height_position.x, height_position.y, 16);
				float old_height = this->height_phash[index];

				if (cloud_position.y > old_height) {
					this->height_phash[index] = cloud_position.y;
				}

				if (cloud_position.y >= collision_bounds && height_position.y < this->proximity_phash[(int)height_position.x]) {
					this->proximity_phash[(int)height_position.x] = height_position.y;
				}
			}
		}

		byte* texture = new byte[4 * 32 * 32];
		for (int i = 0; i < 32; i++) {
			for (int j = 0; j < 32; j++) {
				Vector2 texture_position{ i, j };
				unsigned long long texture_index = Indexer::FlatIndex3(0, i, 31 - j, 4, 32);

				Vector2 height_position = (texture_position / 2).Floor();
				unsigned long long height_index = Indexer::FlatIndex2(height_position.x, height_position.y, 16);

				float height_value = this->height_phash[height_index];

				float height_pixel = 255;
				if (height_value < collision_bounds) {
					height_pixel = 0;
				}

				texture[texture_index] = (byte)height_pixel;
				texture[texture_index + 1] = (byte)height_pixel;
				texture[texture_index + 2] = (byte)height_pixel;
				texture[texture_index + 3] = 255;;
			}
		}

		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(this->height_texture, texture, 4 * 32 * 32, cudaMemcpyHostToDevice), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
	}
};