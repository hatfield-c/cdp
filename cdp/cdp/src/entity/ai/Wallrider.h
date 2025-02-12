#pragma once

#include "../../engine/Transform.h"
#include "PlanStep.h"

struct Wallrider {
	// stages are 0:wallride, 1:transit
	int current_stage = 0;

	float forward_distance = 0;
	int left_score = 0;
	int right_score = 0;
	Vector3 closest_forward{};

	int plan_index = 0;
	int plan_size = 1;
	PlanStep plan[1] = {
		PlanStep{ -1, "./data/wallrider/p00.phash", "" },
		//PlanStep{ 1, "", "" },
		//PlanStep{ 1, "", "" },
	};

	Vector2 phash_size{ 16, 16 };
	int phash_count = 16 * 16;

	void Init() {
		for (int i = 0; i < this->plan_size; i++) {
			PlanStep plan_step = this->plan[i];
			plan_step.Init();
		}
	}

	void Update(Vector3* camera_cloud) {
		Vector3 left_proximity{ sqrt(2) / 2, 0, sqrt(2) / 2 };
		Vector3 right_proximity{ -sqrt(2) / 2, 0, sqrt(2) / 2 };
		left_proximity = left_proximity * 2;
		right_proximity = right_proximity * 2;

		float proximity_radius = 1;

		float forward_distance = 20;
		float forward_radius = 1;
		int forward_count = 0;
		float forward_y_bias = 1;

		float closest_ray_distance = 999999999;
		this->left_score = 0;
		this->right_score = 0;
		for (int i = 0; i < 256; i++) {
			Vector3 point = camera_cloud[i];

			float ray_distance = Transform::Norm3(point);
			float left_distance = Transform::Norm3(point - left_proximity);
			float right_distance = Transform::Norm3(point - right_proximity);

			if (ray_distance < closest_ray_distance && point.y >= 1) {
				closest_ray_distance = ray_distance;
				this->closest_forward = point;
			}

			if (left_distance < proximity_radius) {
				this->left_score++;
			}

			if (right_distance < proximity_radius) {
				this->right_score++;
			}

			Vector2 pixel = Indexer::InverseFlatIndex2(i, 16);
			float line_distance = Transform::Norm2(Vector2{ point.z, point.y - forward_y_bias });

			if (line_distance < forward_radius) {
				if (forward_count == 0) {
					forward_distance = point.x;
				}
				else {
					forward_distance += point.x;
				}

				forward_count++;
			}
		}

		if (forward_count > 0) {
			forward_distance = forward_distance / forward_count;
		}

		this->forward_distance = forward_distance;
		//printf("%.2f\n", this->forward_distance);
	}

	Vector3 GetCommand(float* depth_phash, Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 0 };
		
		if (this->IsSafe(depth_phash, camera_cloud)) {
			if (this->current_stage == 0) {
				command = this->DoWallride(depth_phash, camera_cloud);
			}
			else if (this->current_stage == 1) {
				command = this->DoTransit(depth_phash, camera_cloud);
			}
		}
		
		return command;
	}

	bool IsSafe(float* depth_phash, Vector3* camera_cloud) {
		// change to Vector3 command return
		// brake if there's an emergency. otherwise, check if there are walls on either side. if so, fly in the middle
		
		if (this->plan_index >= this->plan_size) {
			return false;
		}

		return true;
	}

	Vector3 DoWallride(float* depth_phash, Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 1 };
		//Vector3 command{ 0, 0, 0 };

		float proximity_threshold = 5;
		float forward_margin = 0.3;

		PlanStep plan_step = this->plan[this->plan_index];

		bool is_proximity = this->left_score > proximity_threshold;
		if (plan_step.wall_direction > 0) {
			is_proximity = this->right_score > proximity_threshold;
		}

		command[0] = plan_step.wall_direction;
		if (is_proximity || this->forward_distance < 4) {
			command[0] = -plan_step.wall_direction;
		}
		
		Vector3 correlation_data = this->FindSubImage(depth_phash, plan_step.start_phash_cpu, Vector2{ 7, 7 }, Vector2{ 4, 4 });
		float correlation = correlation_data.z;

		correlation_data.Print();

		if(correlation > 0.85 && this->plan_index < this->plan_size - 1){
			this->current_stage++;
		}

		return command;
	}

	Vector3 DoTransit(float* depth_phash, Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 1 };
		PlanStep plan_step = this->plan[this->plan_index];
		float forward_margin = 0.3;
		/*
		if (this->forward_distance > plan_step.z - forward_margin && this->forward_distance < plan_step.z + forward_margin) {
			this->current_stage = 0;
			
			this->plan_index++;

			return command;
		}
		*/
		if (this->closest_forward.z > 0) {
			command.x = -1;
		}

		if (this->closest_forward.z < 0) {
			command.x = 1;
		}

		return command;
	}

	Vector3 FindSubImage(float* camera_phash, float* condition_phash, Vector2 condition_center, Vector2 kernel_radius) {
		Vector3 sub_center{ 7, 7, 0 };
		Vector2 origin = kernel_radius;
		Vector2 endgin{ this->phash_size.x - kernel_radius.x - 1, this->phash_size.y - kernel_radius.y - 1 };

		float highest_correlation = 0;
		for (int j = origin.y; j < endgin.y; j++) {
			for (int i = origin.x; i < endgin.x; i++) {
				Vector2 camera_center{ i, j };
				float correlation = this->GetCorrelation(camera_phash, condition_phash, camera_center, condition_center, kernel_radius);

				if (correlation > highest_correlation) {
					highest_correlation = correlation;
					sub_center.x = camera_center.x;
					sub_center.y = camera_center.y;
					sub_center.z = correlation;
				}
			}
		}

		return sub_center;
	}

	float GetCorrelation(float* camera_phash, float* condition_phash, Vector2 camera_center, Vector2 condition_center, Vector2 kernel_radius) {
		float correlation = 0;
		float camera_norm = 0;
		float condition_norm = 0;

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

				correlation += camera_depth * condition_depth;
				camera_norm += camera_depth * camera_depth;
				condition_norm += condition_depth * condition_depth;
			}
		}

		camera_norm = sqrt(camera_norm);
		condition_norm = sqrt(condition_norm);
		float norm = camera_norm * condition_norm;

		if (norm > 0) {
			correlation = correlation / norm;
		}

		return correlation;
	}
};