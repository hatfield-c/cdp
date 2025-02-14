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
		PlanStep{ -1, "data/wallrider/p0001.proximity", 7 }
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

		if (plan_step.IsStartValid()) {
			Vector2 search_data = this->FindSubImage(this->proximity_phash, plan_step.start_proximity_hash, plan_step.target_center, 4);

			if (search_data.y > 0.95) {
				this->current_stage++;
				command[0] = 0;
			}
		}
		
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

	Vector2 FindSubImage(float* camera_proximity, float* target_proximity, int target_center, int kernel_radius) {
		Vector2 sub_center{ 7 , -1 };
		int origin = kernel_radius;
		int endgin = this->phash_size.x - kernel_radius;

		for (int i = origin; i < endgin; i++) {
			int camera_index = i;
			float correlation = this->GetCorrelation(camera_proximity, target_proximity, camera_index, target_center, kernel_radius);

			if (correlation > sub_center.y) {
				sub_center.x = camera_index;
				sub_center.y = correlation;
			}
		}

		return sub_center;
	}

	float GetCorrelation(float* camera_proximity, float* target_proximity, int camera_center, int target_center, int kernel_radius) {
		float correlation = 0;

		float camera_mean = 0;
		float target_mean = 0;
		for (int i = 0; i < this->phash_size.x; i++) {
			camera_mean += camera_proximity[i];
			target_mean += target_proximity[i];
			
		}
		camera_mean = camera_mean / this->phash_size.x;
		target_mean = target_mean / this->phash_size.x;

		float camera_norm = 0;
		float target_norm = 0;
		for (int i = -kernel_radius; i < kernel_radius; i++) {
			int camera_index = camera_center + i;
			int target_index = target_center + i;

			if (camera_index < 0 || camera_index > this->phash_size.x - 1) {
				continue;
			}

			if (target_index < 0 || target_index > this->phash_size.x - 1) {
				continue;
			}

			float camera_depth = camera_proximity[camera_index] - camera_mean;
			float target_depth = target_proximity[target_index] - target_mean;
			
			camera_norm += camera_depth * camera_depth;
			target_norm += target_depth * target_depth;
			correlation += camera_depth * target_depth;
		}
		camera_norm = sqrt(camera_norm);
		target_norm = sqrt(target_norm);

		correlation = correlation / (camera_norm * target_norm);

		return correlation;
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