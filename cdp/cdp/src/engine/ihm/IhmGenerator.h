#pragma once

#include "../../system/CudaError.h"

#include "../WorldSpace.h"
#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"

struct IhmGenerator {
	int direction_count;
	unsigned long long voxel_count;
	unsigned long long index_count;
	Vector3 world_size{};
	Vector3 position_buffer{};
	Vector4 rotation_buffer{};
	Vector3* directions_cpu;
	Vector3* directions;

	void Init(int segment_count, unsigned long long voxel_count, Vector3 world_size) {
		this->GenerateDirections(segment_count);
		this->voxel_count = voxel_count;
		this->world_size = world_size;
		this->index_count = this->voxel_count * ((unsigned long long)this->direction_count);
		printf("[Direction Count]: %d\n", this->direction_count);

		int memory_size = this->direction_count * sizeof(Vector3);

		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->directions, memory_size), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(this->directions, this->directions_cpu, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
	}

	void SetIhmIndex(unsigned long long position_index, bool is_gpu) {
		Vector4 ihm_state = Indexer::InverseFlatIndex4(position_index, this->direction_count, this->world_size.x, this->world_size.y);

		this->position_buffer.x = ihm_state.y;
		this->position_buffer.y = ihm_state.z;
		this->position_buffer.z = ihm_state.w;

		int direction_index = ihm_state.x;
		Vector3 direction{};

		if (is_gpu) {
			direction = this->directions[direction_index];
		}
		else {
			direction = this->directions_cpu[direction_index];
		}

		this->rotation_buffer = Quaternion::QuaternionFromDirection(direction);
	}

	void GenerateDirections(int segment_count) {
		
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