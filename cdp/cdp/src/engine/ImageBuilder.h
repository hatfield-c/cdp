#pragma once

#include "cuda.h"
#include <curand.h>

#include "../engine/Transform.h"
#include "../engine/Quaternion.h"
#include "../engine/Indexer.h"

typedef unsigned char byte;

struct ImageBuilder {

	void Init() {
		
	}

	__host__ __device__ void WritePixel(byte* img, Vector2 img_size, Vector2 position, Vector3 rgb, bool is_invert_y = true) {
		int y_position = position.y;

		if (is_invert_y) {
			y_position = img_size.y - position.y - 1;
		}

		unsigned long long r_index = Indexer::FlatIndex3(0, position.x, y_position, 3, img_size.x);
		unsigned long long g_index = Indexer::FlatIndex3(1, position.x, y_position, 3, img_size.x);
		unsigned long long b_index = Indexer::FlatIndex3(2, position.x, y_position, 3, img_size.x);

		img[r_index] = rgb.x;
		img[g_index] = rgb.y;
		img[b_index] = rgb.z;
	}

	__host__ __device__ void DrawLine_Serial(byte* img, Vector2 img_size, Vector2 start, Vector2 end, Vector3 rgb, bool is_invert_y = true) {
		Vector2 current_position = start;
		Vector2 current_pixel = start.Floor();

		Vector2 pixel_difference = end.Floor() - start.Floor();
		Vector2 pixel_distance = pixel_difference.Absolute();
		Vector2 difference_sign = pixel_difference.Sign();

		if (pixel_difference.x == 0 && pixel_difference.y == 0) {
			this->WritePixel(img, img_size, start, rgb, is_invert_y);
			return;
		}

		int driving_axis = 0;
		int second_axis = 1;

		if (pixel_distance.y >= pixel_distance.x) {
			driving_axis = 1;
			second_axis = 0;
		}

		float second_slope = pixel_difference[second_axis] / pixel_difference[driving_axis];
		float second_bias = start[second_axis] - (start[driving_axis] * second_slope);
		
		int driving_distance = 0;
		while (driving_distance <= pixel_distance[driving_axis]) {
			driving_distance++;
			current_position[driving_axis] += difference_sign[driving_axis];
			current_position[second_axis] = (current_position[driving_axis] * second_slope) + second_bias;

			if (!current_position.IsBounded(Vector::ZERO2(), img_size - 1)) {
				break;
			}

			current_pixel = current_position.Floor();

			this->WritePixel(img, img_size, current_pixel, rgb, is_invert_y);
		}

	}

};