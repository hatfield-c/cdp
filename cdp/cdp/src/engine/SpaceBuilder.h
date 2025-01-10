#pragma once

#include "cuda.h"
#include <curand.h>
#include <curand_kernel.h>

#include "../engine/Physics.h"
#include "../engine/Transform.h"
#include "../engine/Quaternion.h"
#include "../engine/Indexer.h"

typedef unsigned char byte;

struct SpaceBuilder {

	unsigned long long seed;

	float large_subtraction_p = 0.000003;
	float mid_subtraction_p = 0.00005;
	float small_subtraction_p = 0.001;
	float large_subtraction_radius = 30;
	float mid_subtraction_radius = 10;
	float small_subtraction_radius = 3;

	VoxelData empty_voxel{ 0, 0 };

	void Init() {
		this->seed = 0;
	}

	__device__ void StochasticSubtraction(SpaceData space_data) {
		int voxel_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);

		if (voxel_index >= space_data.voxel_count) {
			return;
		}

		if (threadIdx.x == 0 && blockIdx.x % ((int)(gridDim.x / 20)) == 0) {
			printf("*");
		}

		if (space_data.space_cuda[voxel_index].entity_id == 0) {
			return;
		}

		Vector3 voxel_position = Indexer::InverseFlatIndex3(voxel_index, space_data.world_size.x, space_data.world_size.y);

		curandState_t curand_state;
		unsigned long sequence = voxel_index;
		curand_init(seed, sequence, 0, &curand_state);

		float dice_roll = curand_uniform(&curand_state);

		if (dice_roll < large_subtraction_p) {
			this->AssignSphere(space_data, voxel_position, this->large_subtraction_radius, this->empty_voxel);
		}
		else{
			dice_roll = curand_uniform(&curand_state);

			if (dice_roll < mid_subtraction_p) {
				this->AssignSphere(space_data, voxel_position, this->mid_subtraction_radius, this->empty_voxel);
			}
			else{
				dice_roll = curand_uniform(&curand_state);

				if (dice_roll < small_subtraction_p) {
					this->AssignSphere(space_data, voxel_position, this->small_subtraction_radius, this->empty_voxel);
				}
			}

		}
		
	}

	__device__ void AssignSphere(SpaceData space_data, Vector3 origin, int radius, VoxelData voxel_data) {
		Vector3 brush_position{};

		for (int i = -radius; i <= radius; i++) {
			for (int j = -radius; j <= radius; j++) {
				for (int k = -radius; k <= radius; k++) {
					brush_position.x = origin.x + i;
					brush_position.y = origin.y + j;
					brush_position.z = origin.z + k;

					if (
						brush_position.x < 0 ||
						brush_position.y < 0 ||
						brush_position.z < 0 ||
						brush_position.x >= space_data.world_size.x ||
						brush_position.y >= space_data.world_size.y ||
						brush_position.z >= space_data.world_size.z
					) {
						continue;
					}

					float distance = Transform::Norm3(Vector3{ (float)i, (float)j, (float)k });

					if (distance > radius) {
						continue;
					}

					unsigned long long voxel_index = Indexer::FlatIndex3(brush_position.x, brush_position.y, brush_position.z, space_data.world_size.x, space_data.world_size.y);

					space_data.space_cuda[voxel_index] = this->empty_voxel;
				}
			}
		}
	}

	__device__ void PlanarDensify(SpaceData space_data, Vector3* points, int point_count) {
		int point_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);

		if (point_index >= point_count) {
			return;
		}

		int planar_width = 10;

		Vector3 anchor = points[point_index];
		Vector3 xy_buffer;
		Vector3 xz_buffer;
		Vector3 yz_buffer;

		unsigned long long anchor_index = Indexer::FlatIndex3(anchor.x, anchor.y, anchor.z, space_data.world_size.x, space_data.world_size.y);
		VoxelData voxel_data_buffer;

		for (int i = -planar_width; i <= planar_width; i++) {
			for (int j = -planar_width; j <= planar_width; j++) {

				if (i == 0 && j == 0) {
					continue;
				}

				xy_buffer.x = anchor.x + i;
				xy_buffer.y = anchor.y + j;
				xy_buffer.z = anchor.z;

				xz_buffer.x = anchor.x + i;
				xz_buffer.y = anchor.y;
				xz_buffer.z = anchor.z + j;

				yz_buffer.x = anchor.x;
				yz_buffer.y = anchor.y + i;
				yz_buffer.z = anchor.z + j;

				if (
					xy_buffer.x < 0 ||
					xy_buffer.y < 0 ||
					xy_buffer.z < 0 ||
					xy_buffer.x >= space_data.world_size.x ||
					xy_buffer.y >= space_data.world_size.y ||
					xy_buffer.z >= space_data.world_size.z ||
					//
					xz_buffer.x < 0 ||
					xz_buffer.y < 0 ||
					xz_buffer.z < 0 ||
					xz_buffer.x >= space_data.world_size.x ||
					xz_buffer.y >= space_data.world_size.y ||
					xz_buffer.z >= space_data.world_size.z ||
					//
					yz_buffer.x < 0 ||
					yz_buffer.y < 0 ||
					yz_buffer.z < 0 ||
					yz_buffer.x >= space_data.world_size.x ||
					yz_buffer.y >= space_data.world_size.y ||
					yz_buffer.z >= space_data.world_size.z
				) {
					continue;
				}

				unsigned long long xy_index = Indexer::FlatIndex3(xy_buffer.x, xy_buffer.y, xy_buffer.z, space_data.world_size.x, space_data.world_size.y);
				unsigned long long xz_index = Indexer::FlatIndex3(xz_buffer.x, xz_buffer.y, xz_buffer.z, space_data.world_size.x, space_data.world_size.y);
				unsigned long long yz_index = Indexer::FlatIndex3(yz_buffer.x, yz_buffer.y, yz_buffer.z, space_data.world_size.x, space_data.world_size.y);

				voxel_data_buffer = space_data.space_cuda[xy_index];

				if (voxel_data_buffer.entity_id > 0) {
					this->FillLine(space_data, anchor, xy_buffer, voxel_data_buffer);
				}

				voxel_data_buffer = space_data.space_cuda[xz_index];

				if (voxel_data_buffer.entity_id > 0) {
					this->FillLine(space_data, anchor, xz_buffer, voxel_data_buffer);
				}

				voxel_data_buffer = space_data.space_cuda[yz_index];

				if (voxel_data_buffer.entity_id > 0) {
					this->FillLine(space_data, anchor, yz_buffer, voxel_data_buffer);
				}
			}
		}
	}

	__device__ void FillLine(SpaceData space_data, Vector3 point_a, Vector3 point_b, VoxelData fill_data) {
		Vector3 unit_direction = point_b - point_a;
		unsigned long long step_count = ceil(Transform::Norm3(unit_direction));
		unit_direction = Transform::Unit3(unit_direction);

		Vector3 brush_position{ point_a.x, point_a.y, point_a.z };
		for (int i = 0; i < step_count; i++) {
			if (
				brush_position.x < 0 ||
				brush_position.y < 0 ||
				brush_position.z < 0 ||
				brush_position.x >= space_data.world_size.x ||
				brush_position.y >= space_data.world_size.y ||
				brush_position.z >= space_data.world_size.z
				) {
				break;
			}

			unsigned long long brush_index = Indexer::FlatIndex3(brush_position.x, brush_position.y, brush_position.z, space_data.world_size.x, space_data.world_size.y);
			space_data.space_cuda[brush_index] = fill_data;

			brush_position = brush_position + unit_direction;
		}
	}

};