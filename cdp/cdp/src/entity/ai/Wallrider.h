#pragma once

#include "../../engine/Transform.h"

struct Wallrider {
	// stages are 0:wallride, 1:transit
	int current_stage = 0;

	float forward_distance = 0;
	int left_score = 0;
	int right_score = 0;
	Vector3 closest_forward{};

	int plan_index = 0;
	int plan_size = 3;
	Vector3 plan[3] = {
		Vector3{ -1, 9.98, 3.5 },
		Vector3{ 1, 10, 3.5 },
		Vector3{ -1, 99, 99 }
	};

	void Init() {

	}

	// todo: change condition to use saved point cloud chamfer distance rather than naive distance geoemtry
	//		when getting chamfer distance, *only* compare points greater than a minimum distance. this will
	//		reduce the impact of noise or if the drone is slightly too close to the obstacle
	//		also add proximity turning during transition
	//		when transiting and aligning take a weighted average of filtered points above 1 unit and at least 5 meters out, where taller points have greater weight
	//			need a better transition system. if wall on the right is 5 meters close but very tall, will skew
	//			
	//			use optical flow for target alignment

	void Update(Vector3* camera_cloud) {
		Vector3 plan_step = this->plan[this->plan_index];

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
	}

	Vector3 GetCommand(Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 0 };
		
		if (this->IsSafe(camera_cloud)) {
			if (this->current_stage == 0) {
				command = this->DoWallride(camera_cloud);
			}
			else if (this->current_stage == 1) {
				command = this->DoTransit(camera_cloud);
			}
		}
		
		return command;
	}

	bool IsSafe(Vector3* camera_cloud) {
		// change to Vector3 command return
		// brake if there's an emergency. otherwise, check if there are walls on either side. if so, fly in the middle
		
		if (this->plan_index >= this->plan_size) {
			return false;
		}

		return true;
	}

	Vector3 DoWallride(Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 1 };
		//Vector3 command{ 0, 0, 0 };

		float proximity_threshold = 5;
		float forward_margin = 0.3;

		Vector3 plan_step = this->plan[this->plan_index];

		bool is_proximity = this->left_score > proximity_threshold;
		if (plan_step.x > 0) {
			is_proximity = this->right_score > proximity_threshold;
		}

		command[0] = plan_step.x;
		if (is_proximity || this->forward_distance < 4) {
			command[0] = -plan_step.x;
		}
		
		if (this->forward_distance > plan_step.y - forward_margin && this->forward_distance < plan_step.y + forward_margin) {
			this->current_stage++;
		}

		return command;
	}

	Vector3 DoTransit(Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 1 };
		Vector3 plan_step = this->plan[this->plan_index];
		float forward_margin = 0.3;

		if (this->forward_distance > plan_step.z - forward_margin && this->forward_distance < plan_step.z + forward_margin) {
			this->current_stage = 0;
			
			this->plan_index++;

			return command;
		}

		if (this->closest_forward.z > 0) {
			command.x = -1;
		}

		if (this->closest_forward.z < 0) {
			command.x = 1;
		}

		return command;
	}
};