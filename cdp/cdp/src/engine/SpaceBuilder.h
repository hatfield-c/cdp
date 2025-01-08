#pragma once

#include "cuda.h"

#include "../engine/WorldSpace.h"
#include "../engine/Physics.h"
#include "../engine/Transform.h"
#include "../engine/Quaternion.h"
#include "../engine/Indexer.h"

typedef unsigned char byte;

struct SpaceBuilder {

	void Init() {

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