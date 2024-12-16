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
	Vector2 render_stride;
	Vector4 render_rotation;
	float render_height;
	unsigned long long pixel_count;

	void Init(Vector2 render_size, Vector2 render_stride, Vector4 render_rotation, float render_height) {
		this->render_size = render_size;
		this->render_stride = render_stride;
		this->render_rotation = render_rotation;
		this->render_height = render_height;
		this->pixel_count = render_size.x * render_size.y;
	}

	__device__ void RenderHeatmap(SpaceData space_data, Camera* camera, IhmGenerator ihm_generator, byte* ihm, void(*SyncThreads)()) {
		Vector2 phash_position{
			Indexer::FlatIndex2((unsigned long long)threadIdx.y, (unsigned long long)blockIdx.y, (unsigned long long)blockDim.y),
			Indexer::FlatIndex2((unsigned long long)threadIdx.z, (unsigned long long)blockIdx.z, (unsigned long long)blockDim.z)
		};

		if (phash_position.x == phash_position.y && blockIdx.x == 0) {
			printf("*");
		}

		if (phash_position.x >= camera->phash_data_size.x || phash_position.y >= camera->phash_data_size.y) {
			return;
		}

		Vector2 stride = (camera->camera_size - 1) / (camera->phash_data_size - 1);
		stride.x = (int)stride.x;
		stride.y = (int)stride.y;

		Vector2 pixel_position = phash_position * stride;
		pixel_position.x = (int)pixel_position.x;
		pixel_position.y = (int)pixel_position.y;

		Vector3 render_position{
			pixel_position.x * this->render_stride.x,
			pixel_position.y * this->render_stride.y,
			this->render_height
		};

		unsigned long long data_index = Indexer::FlatIndex3(phash_position.x, phash_position.y, blockIdx.x, 16, 16);

		Vector3 ray_direction = Camera::GetCameraRayDirection(pixel_position, camera->camera_size, camera->fov, this->render_rotation);
		RaycastHitData hit_data = Physics::Raycast(space_data, render_position, ray_direction, pixel_position, camera->max_render_distance);

		Vector3 direction_buffer;
		RaycastHitData depth_data_buffer;

		int width = 1;
		int vote_threshold = 4;
		int depth_votes = 0;
		for (int i = -width; i < (width + 1); i++) {
			for (int j = -width; j < (width + 1); j++) {

				if (i == 0 && j == 0) {
					continue;
				}

				Vector2 camera_query_position{ pixel_position.x + i, pixel_position.y + j };

				if (camera_query_position.x < 0 || camera_query_position.y < 0 || camera_query_position.x >= camera->camera_size.x || camera_query_position.y >= camera->camera_size.y) {
					continue;
				}

				direction_buffer = Camera::GetCameraRayDirection(camera_query_position, camera->camera_size, camera->fov, this->render_rotation);
				depth_data_buffer = Physics::Raycast(space_data, render_position, direction_buffer, camera_query_position, camera->max_render_distance);

				float depth_delta = hit_data.distance - depth_data_buffer.distance;
				if (depth_delta > 0) {
					depth_votes++;
				}
			}
		}

		if (depth_votes >= vote_threshold) {
			ihm[data_index] = 1;
		}

		SyncThreads();
	}

};