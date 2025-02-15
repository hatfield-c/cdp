#pragma once

#include "../../engine/Transform.h"
#include "../../engine/Math.h"
#include "PlanStep.h"

struct Wallrider {
	// stages are 0: anchor, 1:wallride, 2:transit
	int current_stage = 0;
	int plan_index = 0;
	int plan_size = 4;
	PlanStep plan[4] = {
		PlanStep{ -1, "data/wallrider/p0001.proximity", 7 },
		PlanStep{ 1, "data/wallrider/p0002.proximity", 7 },
		PlanStep{ -1, "data/wallrider/p0003.proximity", 7 },
		PlanStep{ 1, "", 7 }
	};

	Vector2 phash_size{ 16, 16 };
	int phash_count = 16 * 16;
	float max_distance = 20.0;

	float* height_phash = new float[16 * 16];
	byte* height_texture;
	float* proximity_phash = new float[16];
	float* proximity_target = new float[16];
	int transit_center = 7;
	
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
				command = this->DoAnchor();
			}
			
			if (this->current_stage == 1) {
				command = this->DoWallride();
			}
			
			if (this->current_stage == 2) {
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

	Vector3 DoAnchor() {
		Vector3 command{ 0, 0, 1 };

		PlanStep plan_step = this->plan[this->plan_index];
		command.x = -plan_step.wall_direction;

		int proximity_index = 1 + plan_step.wall_direction;

		if (!this->steer_proximity[1] && this->steer_proximity[proximity_index]) {
			this->NextStage();
		}

		return command;
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
			Vector2 search_data = this->SadMatch(this->proximity_phash, plan_step.start_proximity_hash, plan_step.target_center, 4);

			if (search_data.y < 0.5) {
				this->NextStage();
				this->transit_center = search_data.x;
				memcpy(this->proximity_target, this->proximity_phash, this->phash_size.x * sizeof(float));

				command[0] = 0;
			}
		}
		
		return command;
	}

	Vector3 DoTransit() {
		Vector3 command{ 0, 0, 1 };
		
		PlanStep plan_step = this->plan[this->plan_index];
		int kernel_radius = 4;

		Vector2 search_result = this->SadMatch(this->proximity_phash, this->proximity_target, this->transit_center, kernel_radius);
		
		this->transit_center = search_result.x;
		float steer_error = 0;
		float steer_direction = 0;
		if (search_result.x < 7) {
			steer_error = 7 - search_result.x;
			steer_direction = -1;
		}
		else if (search_result.x > 8) {
			steer_error = search_result.x - 8;
			steer_direction = 1;
		}

		command.x = steer_direction;

		if (steer_error < 1) {
			memcpy(this->proximity_target, this->proximity_phash, this->phash_size.x * sizeof(float));
		}

		float depth_center = this->proximity_phash[this->transit_center];
		if (steer_error < 2 && depth_center < 4.0 && this->steer_proximity[1]) {
			this->NextPlanStep();
			this->NextStage();

			return command;
		}

		return command;
	}

	Vector2 SadMatch(float* camera_proximity, float* target_proximity, int target_center, int kernel_radius) {
		Vector2 sub_center{ 7 , 999999999 };
		int origin = kernel_radius;
		int endgin = this->phash_size.x - kernel_radius;

		for (int i = origin; i < endgin; i++) {
			int camera_index = i;

			if (camera_proximity[i] > 15) {
				continue;
			}

			float sad_score = this->GetSad(camera_proximity, target_proximity, camera_index, target_center, kernel_radius);

			if (sad_score < sub_center.y) {
				sub_center.x = camera_index;
				sub_center.y = sad_score;
			}
		}

		return sub_center;
	}

	float GetSad(float* camera_proximity, float* target_proximity, int camera_center, int target_center, int kernel_radius) {
		float sad_score = 0;
		int score_count = 0;
		for (int i = -kernel_radius; i < kernel_radius; i++) {
			int camera_index = camera_center + i;
			int target_index = target_center + i;

			if (camera_index < 0 || camera_index > this->phash_size.x - 1) {
				continue;
			}

			if (target_index < 0 || target_index > this->phash_size.x - 1) {
				continue;
			}

			float camera_depth = camera_proximity[camera_index];
			float target_depth = target_proximity[target_index];

			float score = camera_depth - target_depth;
			score = abs(score);

			sad_score += score;
			score_count++;
		}
		
		sad_score = sad_score / score_count;

		return sad_score;
	}

	Vector2 CorrelationMatch(float* camera_proximity, float* target_proximity, int target_center, int kernel_radius) {
		Vector2 sub_center{ 7 , -1 };
		int origin = kernel_radius;
		int endgin = this->phash_size.x - kernel_radius;

		printf("%d\n", target_center);
		for (int i = 0; i < 16; i++) {
			printf("[%.2f]", camera_proximity[i]);
		}
		printf("\n");
		for (int i = 0; i < 16; i++) {
			if (i == target_center) {
				printf("<%.2f>", target_proximity[i]);
			}
			else {
				printf("[%.2f]", target_proximity[i]);
			}
		}
		printf("\n");

		printf("[0.00]");
		printf("[0.00]");
		printf("[0.00]");
		printf("[0.00]");

		for (int i = origin; i < endgin; i++) {
			int camera_index = i;

			if (camera_proximity[i] > 15) {
				continue;
			}

			float correlation = this->GetCorrelation(camera_proximity, target_proximity, camera_index, target_center, kernel_radius);

			printf("[%.2f]", correlation);

			if (correlation > sub_center.y) {
				sub_center.x = camera_index;
				sub_center.y = correlation;
			}
		}

		printf("[0.00]");
		printf("[0.00]");
		printf("[0.00]");
		printf("[0.00]\n");
		sub_center.Print("", "\n\n");

		return sub_center;
	}

	float GetCorrelation(float* camera_proximity, float* target_proximity, int camera_center, int target_center, int kernel_radius) {
		float correlation = 0;

		float camera_mean = 0;
		float target_mean = 0;
		float camera_count = 0;
		float target_count = 0;
		for (int i = 0; i < this->phash_size.x; i++) {
			//if (camera_proximity[i] < 16) {
				camera_mean += camera_proximity[i];
				camera_count++;
			//}

			//if (target_proximity[i] < 16) {
				target_mean += target_proximity[i];
				target_count++;
			//}
			
		}
		camera_mean = camera_mean / camera_count;
		target_mean = target_mean / target_count;

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

	void NextPlanStep() {
		this->plan_index++;
	}

	void NextStage() {
		this->current_stage++;

		if (this->current_stage > 2) {
			this->current_stage = 0;
		}

		printf("[Plan Index: %d][Stage: %d]\n", this->plan_index, this->current_stage);
	}
};