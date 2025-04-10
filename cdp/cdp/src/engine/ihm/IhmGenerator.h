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

	Vector2 pitch_bounds;
	Vector3 world_origin;
	Vector3 world_width;
	Vector3 world_size;
	Vector3 world_stride;
	Vector3 world_width_strided;
	Vector3 world_size_strided;
	Vector2 phash_size;

	Vector3* directions_cpu;
	Vector3* directions;

	void Init(int direction_density, Vector2 pitch_bounds, Vector3 world_origin, Vector3 world_width, Vector3 world_size, Vector3 world_stride, Vector2 phash_size) {
		this->direction_density = direction_density;
		this->pitch_bounds = pitch_bounds;
		this->PreBuildDirections(direction_density);

		this->world_origin = world_origin;
		this->world_width = world_width;
		this->world_size = world_size;
		this->world_stride = world_stride;

		this->world_width_strided = (world_width / world_stride).Floor();
		this->world_width_strided.x = world_width_strided.x;
		this->world_width_strided.y = world_width_strided.y;
		this->world_width_strided.z = world_width_strided.z;

		this->world_size_strided = (world_size / world_stride).Floor();
		this->world_size_strided.x = world_size_strided.x;
		this->world_size_strided.y = world_size_strided.y;
		this->world_size_strided.z = world_size_strided.z;
		
		this->phash_size = phash_size;

		this->phash_count = (unsigned long long)(this->phash_size.x * this->phash_size.y);
		this->voxel_count = (unsigned long long)(this->world_width_strided.x * this->world_width_strided.y * this->world_width_strided.z);
		this->state_count = this->voxel_count * ((unsigned long long)this->direction_count);
		this->bit_count = this->state_count * this->phash_count;
		
		int memory_size = this->direction_count * sizeof(Vector3);
		
		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->directions, memory_size), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(this->directions, this->directions_cpu, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
	}

	__device__ void Generate(SpaceData space_data, Camera* camera, float* ihm) {
		unsigned long long block_index = Indexer::FlatIndex2((unsigned long long)blockIdx.x, (unsigned long long)blockIdx.y, (unsigned long long)gridDim.x);
		unsigned long long block_max = (unsigned long long)(gridDim.x * gridDim.y);

		if (block_index % ((int)(block_max / 20)) == 0 && threadIdx.x == 15 && threadIdx.y == 1) {
			printf("*");
		}

		if (blockIdx.x > 77945) {
			//return;
		}

		IhmState ihm_state = this->GetIhmState(blockIdx.x, true);
		Vector2 phash_position{
			(float)threadIdx.x,
			(float)Indexer::FlatIndex2((unsigned long long)threadIdx.y, (unsigned long long)blockIdx.y, (unsigned long long)blockDim.y)
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

		for (int i = 0; i < (int)camera->chunk_size.x; i += (int)camera->box_filter_stride.x) {
			pixel_position.x = (float)Indexer::FlatIndex2((float)i, phash_position.x, camera->chunk_size.x);

			if (pixel_position.x >= camera->camera_size.x) {
				continue;
			}

			for (int j = 0; j < (int)camera->chunk_size.y; j += (int)camera->box_filter_stride.y) {
				pixel_position.y = (float)Indexer::FlatIndex2((float)j, phash_position.y, camera->chunk_size.y);

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

		unsigned long long data_index = Indexer::FlatIndex3(phash_position.x, phash_position.y, (float)blockIdx.x, camera->phash_data_size.x, camera->phash_data_size.y);
		float depth = avg_distance / space_data.indices_per_meter;

		if (depth > 20.0f) {
			depth = 20.0f;
		}

		ihm[data_index] = depth;
	}

	__device__ void SummationIhm(float* ihm, float* sums) {
		unsigned long long ihm_state_index = Indexer::FlatIndex2((unsigned long long)threadIdx.x, blockIdx.x, blockDim.x);

		if (ihm_state_index >= this->state_count) {
			return;
		}

		float sum = 0;
		for (unsigned long long i = 0; i < 16; i++) {
			for (unsigned long long j = 0; j < 16; j++) {
				unsigned long long data_index = Indexer::FlatIndex3(j, i, ihm_state_index, 16, 16);
				float value = ihm[data_index] / 20.0f;
				sum += value;
			}
		}
		
		sums[ihm_state_index] = sum;
	}

	__device__ void SmoothShm(float* shm, float* buffer) {
		unsigned long long shm_state_index = Indexer::FlatIndex2((unsigned long long)threadIdx.x, blockIdx.x, blockDim.x);

		if (shm_state_index >= this->state_count) {
			return;
		}

		float max_val = 0;
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				unsigned long long data_index = Indexer::FlatIndex3((float)j, i, (float)shm_state_index, 10, 10);
				float value = shm[data_index];

				if (value > max_val) {
					max_val = value;
				}

				float kernel_value = 0;
				for (int y = -5; y < 6; y++) {
					for (int x = -5; x < 6; x++) {
						int x_j = x + j;
						int y_i = y + i;

						if (x_j < 0 || x_j >= 10 || y_i < 0 || y_i >= 10) {
							continue;
						}

						unsigned long long kernel_index = Indexer::FlatIndex3((float)x_j, y_i, (float)shm_state_index, 10, 10);
						float local_value = shm[kernel_index];

						Vector2 max_offset{ x, y };

						float dist = Transform::Norm2(max_offset);
						float mixer = 1 - expf(-0.78 * dist);
						float mixed_value = (mixer * value) + ((1 - mixer) * local_value);

						if (mixed_value > kernel_value) {
							kernel_value = mixed_value;
						}
					}
				}
				
				buffer[data_index] = kernel_value;
			}
		}

		if (max_val == 0) {
			max_val = 1;
		}

		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				unsigned long long data_index = Indexer::FlatIndex3((float)j, i, (float)shm_state_index, 10, 10);

				float value = buffer[data_index] / max_val;

				shm[data_index] = value;
			}
		}
	}

	__device__ void GenerateShm(SpaceData space_data, float* ihm, float* shm, float* buffer, float* sums, void(*SyncThreads)()) {
		Vector2 extracted = Indexer::InverseFlatIndex2((float)blockIdx.x, 10 * 10);

		unsigned long long shm_pixel_index = extracted.x;
		unsigned long long shm_state_index = extracted.y;
		unsigned long long direction_index = threadIdx.x;

		if (shm_state_index != 55020) {
			//return;
		}

		Vector2 shm_pixel = Indexer::InverseFlatIndex2(shm_pixel_index, 10);
		unsigned long long buffer_index = Indexer::FlatIndex2((unsigned long long)threadIdx.x, shm_state_index, blockDim.x);

		float lowest_norm = 1.0f;
		for (unsigned long long i = 0; i < 10; i++) {
			for (unsigned long long j = 0; j < 3; j++) {
				for (unsigned long long k = 0; k < 10; k++) {
					Vector3 ihm_voxel{ i + (shm_pixel.x * 10), j, k + (shm_pixel.y * 10) };

					unsigned long long ihm_state_index = Indexer::FlatIndex4((float)direction_index, ihm_voxel.x, ihm_voxel.y, ihm_voxel.z, this->direction_count, this->world_width_strided.x, this->world_width_strided.y);

					float sum_shm = sums[shm_state_index];
					float sum_ihm = sums[ihm_state_index];
					float diff = abs(sum_shm - sum_ihm) / 256.0f;
					
					if (diff > 0.1f || sum_shm > 250 || sum_ihm > 250  || sum_shm < 5 || sum_ihm < 5) {
						continue;
					}

					float frobenius = 0;
					for (unsigned long long y = 0; y < 16; y++) {
						for (unsigned long long x = 0; x < 16; x++) {
							unsigned long long ihm_data_index = Indexer::FlatIndex3(x, y, ihm_state_index, 16, 16);
							unsigned long long shm_source_index = Indexer::FlatIndex3(x, y, shm_state_index, 16, 16);

							float val0 = ihm[ihm_data_index];
							float val1 = ihm[shm_source_index];

							float difference = (val0 - val1) / 20.0f;
							difference = difference * difference;

							if (ihm_state_index == 524639) {
								//printf("_[%.4f, %.4f] %.4f %.4f\n", val0, val1, difference, frobenius);
							}

							frobenius += difference;
						}
					}

					frobenius = frobenius;

					if (frobenius < 0.30f) {
					//if (shm_pixel.y > 1) {
					//if(ihm_state_index == 524639){
						//printf("\n%.6f %lld - %lld - %lld [%.2f %.2f %.2f] [%.2f %.2f] %lld %lld\n", frobenius, shm_state_index, ihm_state_index, direction_index, ihm_voxel.x, ihm_voxel.y, ihm_voxel.z, shm_pixel.x, shm_pixel.y, i, k);
					}

					if (frobenius < lowest_norm) {
						lowest_norm = frobenius;
					}
				}
			}
		}

		if (blockIdx.x % ((int)(gridDim.x / 20)) == 0 && threadIdx.x == 0) {
			printf("*");
		}

		buffer[buffer_index] = lowest_norm;
		SyncThreads();

		if (threadIdx.x > 0) {
			return;
		}

		lowest_norm = 999999999999;
		for (unsigned long long i = 0; i < blockDim.x; i++) {
			unsigned long long buffer_index = Indexer::FlatIndex2(i, shm_state_index, blockDim.x);

			float norm = buffer[buffer_index];

			if (norm < lowest_norm) {
				lowest_norm = norm;
			}
		}
		
		unsigned long long shm_data_index = Indexer::FlatIndex3(shm_pixel.x, shm_pixel.y, (float)shm_state_index, 10, 10);
		shm[shm_data_index] = 1.0f - lowest_norm;
	}

	__device__ void ExtractRenderClouds(SpaceData space_data, Camera* camera, byte* ihm, Vector3* ihm_clouds) {
		if (blockIdx.x % ((int)(gridDim.x / 20)) == 0 && blockIdx.y == 0 && threadIdx.x == 15 && threadIdx.y == 1) {
			printf("*");
		}

		Vector2 phash_position{
			(float)threadIdx.x,
			(float)Indexer::FlatIndex2((unsigned long long)threadIdx.y, (unsigned long long)blockIdx.y, (unsigned long long)blockDim.y)
		};

		if (!phash_position.IsBounded(Vector::ZERO2(), camera->phash_data_size - 1)) {
			return;
		}

		unsigned long long data_index = Indexer::FlatIndex3(phash_position.x, phash_position.y, (float)blockIdx.x, camera->phash_data_size.x, camera->phash_data_size.y);
		byte depth_val = ihm[data_index];

		IhmState ihm_state = this->GetIhmState(blockIdx.x, true);

		Vector3 quat_dir = Quaternion::RotatePoint(Vector::RIGHT(), ihm_state.rotation);
		Vector3 angles = Quaternion::EulerAnglesFromDirection(quat_dir);
		Vector4 remove_y = Quaternion::QuaternionFromEulerAngles(Vector3{ 0, -angles.y, 0 });

		Vector4 ray_rotation = Quaternion::MultiplyQuaternions(remove_y, ihm_state.rotation, true);
		Vector3 ray_direction = Camera::GetCameraRayDirection(phash_position, this->phash_size, camera->fov, ray_rotation);
		float depth = ((float)depth_val / 256.0f) * camera->max_distance;

		ihm_clouds[data_index] = ray_direction * depth;
	}

	__host__ __device__ IhmState GetIhmState(unsigned long long position_index, bool is_gpu) {
		IhmState ihm_state;
		Vector4 state_data = Indexer::InverseFlatIndex4((float)position_index, (float)this->direction_count, this->world_width_strided.x, this->world_width_strided.y);
		
		ihm_state.position_strided.x = state_data.y;
		ihm_state.position_strided.y = state_data.z;
		ihm_state.position_strided.z = state_data.w;

		ihm_state.position.x = state_data.y * this->world_stride.x;
		ihm_state.position.y = state_data.z * this->world_stride.y;
		ihm_state.position.z = state_data.w * this->world_stride.z;

		ihm_state.position_strided += this->world_origin;
		ihm_state.position += this->world_origin;

		int direction_index = (int)state_data.x;
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

		float y_lower = sin(this->pitch_bounds.x);
		float y_upper = sin(this->pitch_bounds.y);
		float y_diff = abs(y_upper - y_lower);

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
						float y_val = (vertex.y + 1) / 2.0f;
						y_val = y_val * y_diff;
						y_val = y_val + y_lower;
						vertex.y = y_val;

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
		float smallest_norm = 99999999999999.0f;
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