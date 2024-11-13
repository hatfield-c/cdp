#pragma once

#include "cuda.h"
#include "cuda_runtime.h"

#include "Transform.h"
#include "Quaternion.h"
#include "RaycastHitData.h"
#include "SpaceData.h"
#include "Indexer.h"

struct Physics {
    static __device__ RaycastHitData Raycast(SpaceData space_data, Vector3 start_position, Vector3 ray_direction, Vector2 pixel_position, float max_distance) {

        RaycastHitData hit_data{};
        VoxelData voxel_data{};
        Vector3 hit_position{};

        Vector3 query_point = start_position;

        float distance_traveled = 0;
        while (distance_traveled < max_distance) {
            query_point += ray_direction;

            int x_index = (int)query_point.x;
            int y_index = (int)query_point.y;
            int z_index = (int)query_point.z;

            x_index = Transform::Clip(x_index, 0, (int)space_data.world_size.x - 1);
            y_index = Transform::Clip(y_index, 0, (int)space_data.world_size.y - 1);
            z_index = Transform::Clip(z_index, 0, (int)space_data.world_size.z - 1);

            int world_index = Indexer::FlatIndex3(x_index, y_index, z_index, space_data.world_size.x, space_data.world_size.y);

            voxel_data = space_data.space_cuda[world_index];

            hit_position.x = x_index;
            hit_position.y = y_index;
            hit_position.z = z_index;

            hit_data.position = hit_position;
            hit_data.voxel_data = voxel_data;
            hit_data.distance = distance_traveled;

            if (voxel_data.entity_id != 0) {
                break;
            }

            distance_traveled += 1;

        }

        return hit_data;
    }
};