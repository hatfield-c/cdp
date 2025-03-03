#pragma once

#include "../../engine/Transform.h"
#include "../../engine/Math.h"
#include "../../engine/Physics.h"
#include "PlanStep.h"
#include "Pid.h"

struct Wallrider {
	// stages are 0: anchor, 1:wallride, 2:transit
	int current_stage = 0;
	int plan_index = 0;
	int plan_size = 7;
	PlanStep plan[7] = {
		PlanStep{ -1, "data/wallrider/p0001.proximity", 7 },
		PlanStep{ 1, "data/wallrider/p0002.proximity", 7 },
		PlanStep{ -1, "data/wallrider/p0003.proximity", 7 },
		PlanStep{ 1, "data/wallrider/p0004_false.proximity", 7, false },
		PlanStep{ 1, "data/wallrider/p0004.proximity", 7 },
		PlanStep{ -1, "data/wallrider/p0005.proximity", 7 },
		PlanStep{ -1, "", 7 },
	};

	Vector2 phash_size{ 16, 16 };
	int phash_count = 16 * 16;
	float max_distance = 20.0;

	float* height_phash = new float[16 * 16];
	byte* height_texture;
	float* proximity_phash = new float[16];
	float* proximity_blur = new float[16];
	float* proximity_target = new float[16];

	Vector3 velocity{};
	float height_estimate = 4;
	float height_target = 4;
	Pid height_pid{ 0.4, 0, 0.1 };

	Vector3* mass_centers = new Vector3[16];
	
	bool brake_proximity;
	bool* steer_proximity = new bool[3];

	void Init() {
		for (int i = 0; i < this->plan_size; i++) {
			PlanStep plan_step = this->plan[i];
			plan_step.Init();
		}

		for (int i = 0; i < 16; i++) {
			this->proximity_target[i] = 16;
		}
	}

	void Update(float* depth_phash, Vector3* camera_cloud, Vector3 velocity) {
		this->velocity = velocity;
		this->ExtractHeightPhash(depth_phash, camera_cloud);
		this->ExtractMassCenters();
		this->ExtractGroundHeight(depth_phash, camera_cloud, velocity);

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
				if (distance < 4) {
					steer_count[0]++;
				}
			}
			else if (i > 6 && i < 10) {
				if (distance < 4) {
					steer_count[1]++;
				}
			}
			else if (i > 10 && i < 16) {
				if (distance < 4) {
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

	Vector3 GetCommand(Vector3 velocity) {
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

		command.y = this->DoHeight(velocity);

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
			this->UpdateTarget(plan_step);
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
			Vector2 search_data = this->SadMatch(this->proximity_blur, this->proximity_target, plan_step.target_center, 4);

			if (search_data.y < 0.35) {
				for (int i = 0; i < 16; i++) {
					this->proximity_target[i] = 16;
				}

				this->NextStage();

				command[0] = 0;
			}
		}
		
		return command;
	}

	Vector3 DoTransit() {
		Vector3 command{ 0, 0, 1 };
		
		PlanStep plan_step = this->plan[this->plan_index];
		int kernel_radius = 4;

		float steer_error = 0;
		float steer_direction = 0;
		float mass_width = -1;
		Vector3 center_data{ 0, 16, -1 };

		for (int i = 0; i < 16; i++) {
			Vector3 center = this->mass_centers[i];
			
			if (center.z < 0) {
				continue;
			}

			int err = 0;
			int dir = 0;

			if (center.z < 7) {
				err = 7 - center.z;
				dir = -1;
			}
			else if (center.z > 8) {
				err = center.z - 8;
				dir = 1;
			}

			if (center.x > mass_width) {
				mass_width = center.x;
				steer_error = err;
				steer_direction = dir;
				center_data = center;
			}
		}
		
		if (steer_error == 1) {
			steer_direction = 0.25 * steer_direction;
		}

		command.x = steer_direction;

		if (center_data.y < 4.0 && this->steer_proximity[1]) {
			this->NextStage();

			return command;
		}

		return command;
	}

	float DoHeight(Vector3 velocity) {
		float command = 0;

		float height_delta = this->height_target - this->height_estimate;

		command = this->height_pid.ControlStep(this->height_estimate, this->height_target, velocity.y);

		return command;
	}

	Vector2 SadMatch(float* camera_proximity, float* target_proximity, int target_center, int kernel_radius) {
		Vector2 sub_center{ 7 , 999999999 };
		int origin = kernel_radius;
		int endgin = this->phash_size.x - kernel_radius;

		for (int i = origin; i < endgin; i++) {
			if (camera_proximity[i] > 15) {
				continue;
			}

			float sad_score = this->GetSad(camera_proximity, target_proximity, i, target_center, kernel_radius);

			if (sad_score < sub_center.y) {
				sub_center.x = i;
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

		for (int i = origin; i < endgin; i++) {
			int camera_index = i;

			if (camera_proximity[i] > 15) {
				continue;
			}

			float correlation = this->GetCorrelation(camera_proximity, target_proximity, camera_index, target_center, kernel_radius, target_proximity[target_center]);

			if (correlation > sub_center.y) {
				sub_center.x = camera_index;
				sub_center.y = correlation;
			}
		}
		return sub_center;
	}

	float GetCorrelation(float* camera_proximity, float* target_proximity, int camera_center, int target_center, int kernel_radius, float depth_mean) {
		float correlation = 0;

		float camera_mean = 0;
		float target_mean = 0;
		float camera_count = 0;
		float target_count = 0;
		for (int i = 0; i < this->phash_size.x; i++) {
			if (camera_proximity[i] < 16) {
				camera_mean += camera_proximity[i];
				camera_count++;
			}

			if (target_proximity[i] < 16) {
				target_mean += target_proximity[i];
				target_count++;
			}
			
		}
		camera_mean = depth_mean;// camera_mean / camera_count;
		target_mean = depth_mean;// target_mean / target_count;

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

		for (int i = 0; i < 16; i++) {
			int a = i - 1;
			int b = i;
			int c = i + 1;

			if (a < 0) {
				a = 0;
			}

			if (c > 15) {
				c = 15;
			}

			this->proximity_blur[i] = round((this->proximity_phash[a] + this->proximity_phash[b] + this->proximity_phash[c]) / 3);
		}

		float* height_blur = new float[16 * 16];
		memset(height_blur, 0, 16 * 16 * sizeof(float));
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 16; j++) {
				int index = Indexer::FlatIndex2(i, j, 16);

				if (j != this->proximity_blur[i]) {
					height_blur[index] = -1000;
				}
			}
		}
		 
		byte* texture = new byte[4 * 32 * 32];
		for (int i = 0; i < 32; i++) {
			int half_index = floor(i / 2);
			int mass_depth = this->mass_centers[half_index].y;
			int hit_count = 0;

			int target_depth = 16;
			if (this->plan_index < this->plan_size && this->plan[this->plan_index].IsStartValid()) {
				target_depth = this->proximity_target[half_index];
			}
			
			for (int j = 0; j < 32; j++) {
				Vector2 texture_position{ i, j };
				unsigned long long texture_index = Indexer::FlatIndex3(0, i, 31 - j, 4, 32);

				Vector2 height_position = (texture_position / 2).Floor();
				unsigned long long height_index = Indexer::FlatIndex2(height_position.x, height_position.y, 16);

				//float height_value = this->height_phash[height_index];
				float height_value = height_blur[height_index];

				Vector4 height_pixel{ 0, 0, 0, 255 };

				if ((floor(j / 2) == target_depth)) {
					height_pixel = Vector4{ 0, 255, 0, 255 };
				}
				
				if (height_value >= collision_bounds && hit_count < 2) {
					height_pixel = Vector4{ 255, 255, 255, 255 };
					hit_count++;
				}
				
				if (floor(j / 2) == mass_depth && j % 2 == 0) {
					if (this->mass_centers[half_index].z >= 0) {
						height_pixel = Vector4{ 255, 0, 0, 255 };
					}
				}

				texture[texture_index] = (byte)height_pixel.x;
				texture[texture_index + 1] = (byte)height_pixel.y;
				texture[texture_index + 2] = (byte)height_pixel.z;
				texture[texture_index + 3] = 255;
			}
		}

		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(this->height_texture, texture, 4 * 32 * 32, cudaMemcpyHostToDevice), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
	}

	void ExtractMassCenters() {
		for (int i = 0; i < 16; i++) {
			this->mass_centers[i] = Vector3{ 0, 16, -1 };
		}

		float mass_start = 0;
		float mass_end = 16;
		float center_depth = 16;

		float mass_threshold = 1;

		for (int i = 1; i < 16; i++) {
			float depth0 = this->proximity_blur[i - 1];
			float depth1 = this->proximity_blur[i];
			
			if (depth0 < center_depth) {
				center_depth = depth0;
			}

			float delta = depth1 - depth0;

			if (abs(delta) > mass_threshold || (depth0 != 16 && depth1 == 16) || (depth0 == 16 && depth1 != 16) || i == 15) {
				mass_end = i - 1;

				int mass_center = round((mass_end + mass_start) / 2);

				if (center_depth < 16 && mass_center != 0) {
					this->mass_centers[mass_center].x = mass_end - mass_start;
					this->mass_centers[mass_center].y = center_depth;
					this->mass_centers[mass_center].z = mass_center;
				}

				mass_start = i;
				mass_end = 16;
				center_depth = 16;
			}
		}
	}

	void ExtractGroundHeight(float* depth_phash, Vector3* camera_cloud, Vector3 velocity) {
		Vector2 search_radius{ 1, 8 };
		float average_height = 0;
		float height_threshold = 1;
		float height_bias = 0.7;
		int ground_votes = 0;
		int vote_threshold = 4;

		int lowest_index = 0;
		float lowest_height = 99999999;

		for (int i = 0; i < 16; i++) {
			Vector2 phash_position{ i, 15 };
			unsigned long long phash_index = Indexer::FlatIndex2(phash_position.x, phash_position.y, 16);
			Vector3 cloud_position = camera_cloud[phash_index];

			if (cloud_position.y < lowest_height) {
				lowest_height = cloud_position.y;
				lowest_index = i;
			}
		}

		for (int i = lowest_index - search_radius.x; i <= lowest_index + search_radius.x; i++) {
			Vector3 previous_position{ 999999, 999999, 999999 };

			for (int j = 0; j <= search_radius.y; j++) {
				Vector2 phash_position{ i, 15 - j };
				unsigned long long phash_index = Indexer::FlatIndex2(phash_position.x, phash_position.y, 16);
				Vector3 cloud_position = camera_cloud[phash_index];
				float depth = depth_phash[phash_index];
				float height_delta = cloud_position.y - lowest_height;

				float distance = Transform::Norm2(Vector2{ previous_position.x, previous_position.z } - Vector2{ cloud_position.x, cloud_position.z });

				if (distance < 0.5) {
					break;
				}

				if (height_delta < height_threshold && depth < 20.0f) {
					average_height += cloud_position.y;
					ground_votes++;
				}

				previous_position = cloud_position;
			}
		}

		if (ground_votes >= vote_threshold) {
			this->height_estimate = (-average_height / ground_votes) + height_bias;
		}
		else {
			this->height_estimate += velocity.y * Physics::DeltaTime();
		}
	}

	void UpdateTarget(PlanStep plan_step) {
		if (plan_step.IsStartValid()) {
			memcpy(this->proximity_target, plan_step.start_proximity_hash, this->phash_size.x * sizeof(float));
		}
		else {
			for (int i = 0; i < 16; i++) {
				this->proximity_target[i] = 16;
			}
		}
	}

	void NextPlanStep() {
		this->plan_index++;
	}

	void NextStage() {
		PlanStep plan_step = this->plan[this->plan_index];

		if (!plan_step.is_transit && this->current_stage == 1) {
			this->NextPlanStep();
			this->UpdateTarget(this->plan[this->plan_index]);
		}
		else {
			this->current_stage++;
		}

		if (this->current_stage > 2) {
			this->current_stage = 0;
			this->NextPlanStep();
		}

		printf("[Plan Index: %d][Stage: %d]\n", this->plan_index, this->current_stage);
	}
};