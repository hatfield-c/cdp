#pragma once

#include "../../system/CudaError.h"

#include "../Transform.h"

struct IhmGenerator {
	int direction_count;
	Vector3* directions_cpu;
	Vector3* directions;
	
	void Init(int segment_count) {
		this->GenerateDirections(segment_count);

		unsigned long long memory_size = this->direction_count * sizeof(Vector3);

		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->directions, memory_size), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(this->directions, this->directions_cpu, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
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

		this->directions = vertices;
	}
};