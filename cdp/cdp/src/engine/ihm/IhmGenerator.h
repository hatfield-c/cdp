#pragma once

#include "../../system/CudaError.h"

#include "IhmState.h"
#include "../WorldSpace.h"
#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"
#include "../RaycastHitData.h"
#include "../../entity/Camera.h"

struct IhmGenerator {
	int direction_density;
	int direction_count;
	unsigned long long voxel_count;
	unsigned long long state_count;
	unsigned long long phash_count;
	unsigned long long bit_count;

	Vector3 world_origin;
	Vector3 world_width;
	Vector3 world_size;
	Vector3 world_stride;
	Vector3 world_width_strided;
	Vector3 world_size_strided;
	Vector2 phash_size;

	Vector3* directions_cpu;
	Vector3* directions;

	void Init(int direction_density, Vector3 world_origin, Vector3 world_width, Vector3 world_size, Vector3 world_stride, Vector2 phash_size) {
		this->direction_density = direction_density;
		this->PreBuildDirections(direction_density);

		this->world_origin = world_origin;
		this->world_width = world_width;
		this->world_size = world_size;
		this->world_stride = world_stride;

		this->world_width_strided = world_width / world_stride;
		this->world_width_strided.x = (int)world_width_strided.x;
		this->world_width_strided.y = (int)world_width_strided.y;
		this->world_width_strided.z = (int)world_width_strided.z;

		this->world_size_strided = world_size / world_stride;
		this->world_size_strided.x = (int)world_size_strided.x;
		this->world_size_strided.y = (int)world_size_strided.y;
		this->world_size_strided.z = (int)world_size_strided.z;
		
		this->phash_size = phash_size;

		this->phash_count = this->phash_size.x * this->phash_size.y;
		this->voxel_count = this->world_width_strided.x * this->world_width_strided.y * this->world_width_strided.z;
		this->state_count = this->voxel_count * ((unsigned long long)this->direction_count);
		this->bit_count = this->state_count * this->phash_count;
		
		int memory_size = this->direction_count * sizeof(Vector3);
		
		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->directions, memory_size), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(this->directions, this->directions_cpu, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
	}

	__device__ void Generate(SpaceData space_data, Camera* camera, byte* ihm) {

		if (blockIdx.x % ((int)(gridDim.x / 20)) == 0 && blockIdx.y == 0 && threadIdx.x == 15 && threadIdx.y == 1) {
			printf("*");
		}

		IhmState ihm_state = this->GetIhmState(blockIdx.x, true);
		Vector2 phash_position{
			threadIdx.x,
			Indexer::FlatIndex2(threadIdx.y, blockIdx.y, blockDim.y)
		};
		Vector2 pixel_position;

		if (!phash_position.IsBounded(Vector::ZERO2(), camera->phash_data_size - 1)) {
			return;
		}

		unsigned long long world_index = Indexer::FlatIndex3(ihm_state.position.x, ihm_state.position.y, ihm_state.position.z, space_data.world_size0.x, space_data.world_size0.y);
		VoxelData voxel_data = space_data.space0[world_index];

		if (voxel_data.entity_id != 0) {
			return;
		}

		float avg_distance = 0;
		int avg_count = 0;

		for (int i = 0; i < camera->chunk_size.x; i += camera->box_filter_stride.x) {
			pixel_position.x = Indexer::FlatIndex2(i, phash_position.x, camera->chunk_size.x);

			if (pixel_position.x >= camera->camera_size.x) {
				continue;
			}

			for (int j = 0; j < camera->chunk_size.y; j += camera->box_filter_stride.y) {
				pixel_position.y = Indexer::FlatIndex2(j, phash_position.y, camera->chunk_size.y);

				if (pixel_position.y >= camera->camera_size.y) {
					continue;
				}

				Vector3 ray_direction = Camera::GetCameraRayDirection(pixel_position, camera->camera_size, camera->fov, ihm_state.rotation);
				RaycastHitData hit_data = Physics::Raycast(space_data, ihm_state.position, ray_direction, camera->max_render_distance);
				float depth = hit_data.distance;

				if (hit_data.voxel_data.entity_id == 0) {
					depth = camera->max_render_distance;
				}

				avg_distance += depth;
				avg_count++;
			}
		}

		if (avg_count < 1) {
			avg_distance = camera->max_render_distance;
			avg_count = 1;
		}

		avg_distance = avg_distance / avg_count;

		byte phash_pixel_value = camera->DepthToPixel(avg_distance);

		unsigned long long data_index = Indexer::FlatIndex3(phash_position.x, phash_position.y, blockIdx.x, camera->phash_data_size.x, camera->phash_data_size.y);
		ihm[data_index] = phash_pixel_value;
	}

	__device__ void ExtractRenderClouds(SpaceData space_data, Camera* camera, byte* ihm, Vector3* ihm_clouds) {
		
		if (blockIdx.x % ((int)(gridDim.x / 20)) == 0 && blockIdx.y == 0 && threadIdx.x == 15 && threadIdx.y == 1) {
			printf("*");
		}

		Vector2 phash_position{
			threadIdx.x,
			Indexer::FlatIndex2(threadIdx.y, blockIdx.y, blockDim.y)
		};

		if (!phash_position.IsBounded(Vector::ZERO2(), camera->phash_data_size - 1)) {
			return;
		}

		unsigned long long data_index = Indexer::FlatIndex3(phash_position.x, phash_position.y, blockIdx.x, camera->phash_data_size.x, camera->phash_data_size.y);
		byte depth_val = ihm[data_index];

		IhmState ihm_state = this->GetIhmState(blockIdx.x, true);

		Vector3 quat_dir = Quaternion::RotatePoint(Vector::RIGHT(), ihm_state.rotation);
		Vector3 angles = Quaternion::EulerAnglesFromDirection(quat_dir);
		Vector4 remove_y = Quaternion::QuaternionFromEulerAngles(Vector3{ 0, -angles.y, 0 });

		Vector4 ray_rotation = Quaternion::MultiplyQuaternions(remove_y, ihm_state.rotation, true);
		Vector3 ray_direction = Camera::GetCameraRayDirection(phash_position, this->phash_size, camera->fov, ray_rotation);
		float depth = (depth_val / 256.0) * camera->max_distance;

		ihm_clouds[data_index] = ray_direction * depth;
	}

	__host__ __device__ IhmState GetIhmState(unsigned long long position_index, bool is_gpu) {
		IhmState ihm_state;
		Vector4 state_data = Indexer::InverseFlatIndex4(position_index, this->direction_count, this->world_width_strided.x, this->world_width_strided.y);
		
		ihm_state.position_strided.x = state_data.y;
		ihm_state.position_strided.y = state_data.z;
		ihm_state.position_strided.z = state_data.w;

		ihm_state.position.x = state_data.y * this->world_stride.x;
		ihm_state.position.y = state_data.z * this->world_stride.y;
		ihm_state.position.z = state_data.w * this->world_stride.z;

		ihm_state.position_strided += this->world_origin;
		ihm_state.position += this->world_origin;

		int direction_index = state_data.x;
		Vector3 direction{};

		if (is_gpu) {
			direction = this->directions[direction_index];
		}
		else {
			direction = this->directions_cpu[direction_index];
		}

		ihm_state.direction_index = direction_index;
		ihm_state.rotation = Quaternion::QuaternionFromDirection(direction);

		return ihm_state;
	}

	void PreBuildDirections(int segment_count) {
		
		if (segment_count < 3) {
			segment_count = 3;
		}

		if (segment_count % 2 == 0) {
			segment_count++;
		}

		int vertex_count = (2 * segment_count * segment_count);
		vertex_count += (2 * (segment_count) * (segment_count - 2));
		vertex_count += (2 * (segment_count - 2) * (segment_count - 2));
		vertex_count -= 2;
		this->direction_count = vertex_count;

		float step_size = 2.0f / (segment_count - 1);
		int vertex_index = 0;
		Vector3* vertices = new Vector3[vertex_count];

		for (int x = 0; x < segment_count; x++) {
			for (int y = 0; y < segment_count; y++) {
				for (int z = 0; z < segment_count; z++) {
					bool is_added = false;
					Vector3 vertex = {
						(x * step_size) - 1,
						(y * step_size) - 1,
						(z * step_size) - 1,
					};

					if (x == (int)(segment_count / 2) && z == (int)(segment_count / 2)) {
						if (y == 0 || y == segment_count - 1) {
							continue;
						}
					}

					if (x == 0 || x == segment_count - 1) {
						is_added = true;
					}
					else {
						if (y == 0 || y == segment_count - 1) {
							is_added = true;
						}

						if (z == 0 || z == segment_count - 1) {
							is_added = true;
						}
					}

					if (is_added) {
						vertex = Transform::Unit3(vertex);
						vertices[vertex_index] = vertex;
						vertex_index++;
					}
				}
			}
		}

		this->directions_cpu = vertices;
	}

	int GetClosestDirectionIndex(Vector3 unit_vector) {
		int closest_index = 0;
		float smallest_norm = 99999999999999;
		for (int i = 0; i < this->direction_count; i++) {
			Vector3 direction = this->directions_cpu[i];
			float diff_norm = Transform::Norm3(direction - unit_vector);

			if (diff_norm < smallest_norm) {
				closest_index = i;
				smallest_norm = diff_norm;
			}
		}

		return closest_index;
	}
};