#pragma once

#include "../../system/CudaError.h"

#include "IhmState.h"
#include "../WorldSpace.h"
#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"

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
		
		IhmState ihm_state = this->GetIhmState(blockIdx.x, true);
		
		Vector2 stride = (camera->camera_size - 1) / (camera->phash_data_size - 1);
		stride.x = (int)stride.x;
		stride.y = (int)stride.y;

		Vector2 pixel_position = phash_position * stride;
		pixel_position.x = (int)pixel_position.x;
		pixel_position.y = (int)pixel_position.y;
		
		unsigned long long data_index = Indexer::FlatIndex3(phash_position.x, phash_position.y, blockIdx.x, 16, 16);

		Vector3 ray_direction = Camera::GetCameraRayDirection(pixel_position, camera->camera_size, camera->fov, ihm_state.rotation);
		RaycastHitData hit_data = Physics::Raycast(space_data, ihm_state.position, ray_direction, pixel_position, camera->max_render_distance);

		if (((int)hit_data.distance) % 2 == 0) {
			ihm[data_index] = 1;
		}
	}

	__device__ IhmState GetIhmState(unsigned long long position_index, bool is_gpu) {
		IhmState ihm_state;
		Vector4 state_data = Indexer::InverseFlatIndex4(position_index, this->direction_count, this->world_width_strided.x, this->world_width_strided.y);
		
		ihm_state.position.x = state_data.y * this->world_stride.x;
		ihm_state.position.y = state_data.z * this->world_stride.y;
		ihm_state.position.z = state_data.w * this->world_stride.z;
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
};