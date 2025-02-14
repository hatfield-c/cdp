#pragma once

#include "../../engine/Transform.h"
#include "../../engine/Math.h"
#include "PlanStep.h"

struct Wallrider {
	// stages are 0:wallride, 1:transit
	int current_stage = 0;

	float* transit_kernel = new float[16 * 16];
	Vector2 transit_center{ 7, 7 };
	float start_distance_signal = 0;

	float forward_distance = 0;
	int left_score = 0;
	int right_score = 0;
	Vector3 closest_forward{};

	int plan_index = 0;
	int plan_size = 4;
	PlanStep plan[4] = {
		PlanStep{ -1, "./data/wallrider/p_0001.phash", Vector2{ 7, 7 } },
		PlanStep{ 1, "./data/wallrider/p_0002.phash", Vector2{ 5, 7 } },
		PlanStep{ -1, "./data/wallrider/p_0003.phash", Vector2{ 5, 7 } },
		PlanStep{ 1, "", Vector2{ 7, 7 } }
	};

	Vector2 phash_size{ 16, 16 };
	int phash_count = 16 * 16;
	float max_distance = 20.0;

	float* height_phash = new float[256];
	byte* height_texture;

	void Init() {
		for (int i = 0; i < this->plan_size; i++) {
			PlanStep plan_step = this->plan[i];
			plan_step.Init();
		}
	}

	void Update(float* depth_phash, Vector3* camera_cloud) {
		Vector3 left_proximity{ sqrt(2) / 2, 0, sqrt(2) / 2 };
		Vector3 right_proximity{ -sqrt(2) / 2, 0, sqrt(2) / 2 };
		left_proximity = left_proximity * 2;
		right_proximity = right_proximity * 2;

		float proximity_radius = 1;

		float forward_distance = 20;
		float forward_radius = 1.8;
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

		this->ExtractHeightPhash(depth_phash, camera_cloud);
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
		// change to Vector3 command return
		// brake if there's an emergency. otherwise, check if there are walls on either side. if so, fly in the middle

		if (this->plan_index >= this->plan_size) {
			return false;
		}

		return true;
	}

	Vector3 DoWallride() {
		Vector3 command{ 0, 0, 1 };
		/*
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

		if (plan_step.IsStartValid()) {
			Vector3 search_result = this->FindSubImage(depth_phash, plan_step.start_phash_cpu, plan_step.target_center, Vector2{ 4, 4 });
			float difference_score = search_result.z;
			printf("%.6f\n", difference_score);
			if (difference_score < 0.0015) {
				this->current_stage++;
				this->transit_kernel = depth_phash;
				this->transit_center = Vector2{ search_result.x, search_result.y };
				this->start_distance_signal = this->VerticalEdgeAverage(depth_phash, this->transit_center);
			}
		}
		*/
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

	float FirstVerticalEdgeDistance(float* depth_phash, Vector2 target_center) {
		float delta_threshold = 2;

		float vertical_average = 0;

		for (int i = this->phash_size.y - 1; i > 1; i--) {
			unsigned long long index0 = Indexer::FlatIndex2(target_center.x, i, this->phash_size.x);
			unsigned long long index1 = Indexer::FlatIndex2(target_center.x, i - 1, this->phash_size.x);

			float depth0 = depth_phash[index0];
			float depth1 = depth_phash[index1];

			vertical_average += depth0;
			float delta = abs(depth1 - depth0);

			if (delta > delta_threshold) {
				if (depth0 < depth1) {
					return depth0;
				}
				else {
					return depth1;
				}
			}
		}

		return vertical_average / (this->phash_size.y - 1);
	}

	float VerticalEdgeAverage(float* depth_phash, Vector2 target_center) {
		float delta_threshold = 1;

		float vertical_average = 0;
		float edge_average = 0;
		float edge_count = 0;

		for (int i = 0; i < this->phash_size.y - 1; i++) {
			unsigned long long index0 = Indexer::FlatIndex2(target_center.x, i, this->phash_size.x);
			unsigned long long index1 = Indexer::FlatIndex2(target_center.x, i + 1, this->phash_size.x);

			float depth0 = depth_phash[index0];
			float depth1 = depth_phash[index1];

			vertical_average += depth0;
			float delta = abs(depth1 - depth0);

			if (delta > delta_threshold) {
				if (depth0 < depth1) {
					edge_average += depth0;
				}
				else {
					edge_average += depth1;
				}

				edge_count++;
			}
		}

		vertical_average = vertical_average / (this->phash_size.y - 1);

		if (edge_count < 1) {
			return vertical_average;
		}

		return edge_average / edge_count;
	}

	float AverageKernel(float* depth_phash, Vector2 center, Vector2 radius) {
		float avg_distance = 0;
		for (int i = -radius.x; i <= radius.x; i++) {
			for (int j = -radius.y; j <= radius.y; j++) {
				Vector2 offset{ i, j };
				Vector2 pixel_position = this->transit_center + offset;
				unsigned long long pixel_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, this->phash_size.x);

				avg_distance += depth_phash[pixel_index];
			}
		}
		avg_distance = avg_distance / (((2 * radius.x) + 1) * ((2 * radius.y) + 1));

		return avg_distance;
	}

	void ExtractHeightPhash(float* depth_phash, Vector3* camera_cloud) {
		
		for (int i = 0; i < 256; i++) {
			this->height_phash[i] = -1000;
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
			}
		}

		for (int i = 0; i < 16; i++) {
			float last_up_height = -1000;

			for (int j = 15; j >= 0; j--) {
				unsigned long long up_index = Indexer::FlatIndex2(i, j, 16);

				float up_value = this->height_phash[up_index];

				if (up_value >= -4) {
					last_up_height = up_value;
				} else if (last_up_height > -4) {
					this->height_phash[up_index] = last_up_height;
				}
			}

			last_up_height = -1000;
			for (int j = 0; j < 16; j++) {
				unsigned long long down_index = Indexer::FlatIndex2(i, j, 16);

				float up_value = this->height_phash[down_index];

				if (up_value >= -4) {
					last_up_height = up_value;
				}
				else if (last_up_height > -4) {
					this->height_phash[down_index] = last_up_height;
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
				if (height_value >= -4.0) {
					height_pixel = ((height_value + 4) / 15) * 255;
					height_pixel = Math::Clip(height_pixel, 0.0, 255.0);
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