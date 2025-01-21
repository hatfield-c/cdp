#pragma once

#include "../../system/CudaError.h"

#include "IhmGenerator.h"
#include "IhmState.h"
#include "../WorldSpace.h"
#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"

struct IhmRenderer {

	Vector2 render_size;
	Vector2 render_size_strided;
	Vector2 render_stride;
	unsigned long long pixel_count;

	void Init(Vector2 render_size, Vector2 render_stride) {
		this->render_size = render_size;
		this->render_stride = render_stride;

		this->render_size_strided = Vector2{ (float)(int)(render_size.x / render_stride.x), (float)(int)(render_size.y / render_stride.y)};

		this->pixel_count = render_size.x * render_size.y;
	}

	__device__ void RenderSimilarityHeatmap(SpaceData space_data, IhmGenerator ihm_generator, IhmGenerator slice_generator, byte* ihm, byte* ihm_slice, double* score_buffer, byte* img, int direction_index, int height, void(*SyncThreads)()) {
		Vector2 slice_position = Indexer::InverseFlatIndex2(blockIdx.x, slice_generator.world_width_strided.x);
		unsigned long long thread_units = ceil(((double)ihm_generator.voxel_count) / ((double)blockDim.x));

		if (blockIdx.x % ((int)(gridDim.x / 20)) == 0 && threadIdx.x == 0) {
			printf("*");
		}

		if (slice_position.x < 100 || slice_position.y < 440 || slice_position.x > 500 || slice_position.y > 770) {
			return;
		}

		unsigned long long thread_buffer_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);
		unsigned long long slice_state_index = Indexer::FlatIndex4(direction_index, slice_position.x, 0, slice_position.y, slice_generator.direction_count, slice_generator.world_width_strided.x, slice_generator.world_width_strided.y);

		if (slice_state_index >= slice_generator.bit_count) {
			return;
		}

		double thread_score = 0;
		double score_threshold = 51;
		for (unsigned long long i = 0; i < thread_units; i++) {
			unsigned long long ihm_voxel_index = i + (threadIdx.x * thread_units);

			if (ihm_voxel_index >= ihm_generator.voxel_count) {
				break;
			}

			Vector3 voxel_position = Indexer::InverseFlatIndex3(ihm_voxel_index, ihm_generator.world_width_strided.x, ihm_generator.world_width_strided.y);

			unsigned long long ihm_state_index = Indexer::FlatIndex4(direction_index, voxel_position.x, voxel_position.y, voxel_position.z, ihm_generator.direction_count, ihm_generator.world_width_strided.x, ihm_generator.world_width_strided.y);
		
			double phash_score = 0;
			for (int j = 0; j < ihm_generator.phash_size.x; j++) {
				for (int k = 0; k < ihm_generator.phash_size.y; k++) {
					unsigned long long slice_data_index = Indexer::FlatIndex3(k, j, slice_state_index, slice_generator.phash_size.x, slice_generator.phash_size.y);
					unsigned long long ihm_data_index = Indexer::FlatIndex3(k, j, ihm_state_index, ihm_generator.phash_size.x, ihm_generator.phash_size.y);

					//if (slice_position.x == 400 && slice_position.y == 603 && ihm_state_index == 4330572) {
						//byte ihm_test = ihm[ihm_data_index];
						//byte slice_test = ihm_slice[slice_data_index];
						//printf("<%lld %lld %lld> <%lld %lld %lld> - %d %0.2f - %.2f %.2f = %.2f %.2f %.2f | [%d, %d] %d %d %d\n", ihm_state_index, ihm_data_index, ihm_generator.bit_count, slice_state_index, slice_data_index, slice_generator.bit_count, is_same, thread_score, slice_position.x, slice_position.y, voxel_position.x, voxel_position.y, voxel_position.z, k, j, ihm_test, slice_test, ihm_test != slice_test);
					//}

					if (slice_data_index >= slice_generator.bit_count || ihm_data_index >= ihm_generator.bit_count) {
						continue;
					}

					int slice_value = ihm_slice[slice_data_index];
					int ihm_value = ihm[ihm_data_index];
					int value_diff = slice_value - ihm_value;
					value_diff = abs(value_diff);

					phash_score += value_diff;
				}
			}

			if (phash_score < score_threshold) {
				thread_score++;
			}
		}

		score_buffer[thread_buffer_index] = thread_score;

		SyncThreads();

		if (threadIdx.x > 0) {
			return;
		}

		double pixel_score = 0;
		for (int i = 0; i < blockDim.x; i++) {
			unsigned long long thread_score_index = Indexer::FlatIndex2(i, blockIdx.x, blockDim.x);

			pixel_score += score_buffer[thread_score_index];
		}

		pixel_score = log(pixel_score + 1);
		pixel_score = 255 * 0.25 * pixel_score;
		pixel_score = Math::Clip(pixel_score, 0.0, 255.0);

		byte r_val = (int)pixel_score;
		byte g_val = 255 - r_val;
		byte b_val = 0;

		if (pixel_score == 0) {
			r_val = 0;
			g_val = 0;
			b_val = 255;
		}

		Vector2 render_position = slice_position * this->render_stride;

		unsigned long long r_index = Indexer::FlatIndex3(0, render_position.x, render_position.y, 3, this->render_size.x);
		img[r_index] = r_val;
		img[r_index + 1] = g_val;
		img[r_index + 2] = b_val;
	}

};