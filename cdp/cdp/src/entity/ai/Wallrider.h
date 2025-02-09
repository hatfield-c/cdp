#pragma once

#include "../../engine/Transform.h"

struct Wallrider {
	// stages are 0:wallride, 1:transit
	int current_stage = 0;

	// start with left
	float current_forward;

	int plan_index = 0;
	int step_count = 3;
	Vector3 plan[3] = {
		Vector3{ -1, 7.5, 4 },
		Vector3{ 1, 10, 4 },
		Vector3{ -1, 99, 99 }
	};
	

	void Init() {

	}

	Vector3 GetCommand(Vector3* camera_cloud) {
		Vector3 command{};

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
		return true;
	}

	Vector3 DoWallride(Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 1 };
		//Vector3 command{ 0, 0, 0 };

		Vector3 plan_step = this->plan[this->plan_index];

		command[0] = plan_step.x;

		Vector3 left_origin{ sqrt(2) / 2, 0, sqrt(2) / 2 };
		left_origin = left_origin * 2;

		float left_score = 0;
		float left_radius = 1;
		float left_threshold = 5;

		float forward_distance = 20;
		float forward_radius = 1;
		int forward_count = 0;
		float forward_margin = 0.3;

		for (int i = 0; i < 256; i++) {
			Vector3 point = camera_cloud[i];

			float distance = Transform::Norm3(point - left_origin);

			if (distance < left_radius) {
				left_score++;
			}

			Vector2 pixel = Indexer::InverseFlatIndex2(i, 16);
			distance = Transform::Norm2(Vector2{ point.z, point.y });

			if (distance < forward_radius) {
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
		//printf("%.2f %.2f %.2f\n", forward_distance, plan_step.y - forward_margin, plan_step.y + forward_margin);
		if (forward_distance > plan_step.y - forward_margin && forward_distance < plan_step.y + forward_margin) {
			this->current_stage++;
		}

		this->current_forward = forward_distance;

		if (left_score > left_threshold || forward_distance < 4) {
			command[0] = -plan_step.x;
		}

		return command;
	}

	Vector3 DoTransit(Vector3* camera_cloud) {
		Vector3 command{ 0, 0, 1 };

		return command;
	}
};