#pragma once

#include "cuda.h"
#include "cuda_runtime.h"

#include "Math.h"
#include "Transform.h"
#include "Quaternion.h"
#include "RaycastHitData.h"
#include "SpaceData.h"
#include "Indexer.h"

struct Physics {
    static __host__ __device__ float DeltaTime() {
        return 1.0f / 60.0f;
    }

    static __host__ __device__ float DeltaTimeMilli() {
        return Physics::DeltaTime() * 1000;
    }

    static __host__ __device__ Vector3 Gravity() {
        return Vector3{ 0, -9.80665, 0 };
    }

    static __host__ __device__ RaycastHitData Raycast(SpaceData space_data, Vector3 start_position, Vector3 ray_direction, float max_distance) {
        RaycastHitData hit_data{};
        VoxelData voxel_data{};
        Vector3 hit_position{};

        Vector3 end_position = start_position + (ray_direction * max_distance);
        end_position = end_position.Clip(Vector::ZERO3(), space_data.world_size0);

        Vector3 start_voxel = start_position.Floor();
        Vector3 end_voxel = end_position.Floor();

        Vector3 voxel_difference = end_voxel - start_voxel;
        Vector3 voxel_distance = voxel_difference.Absolute();
        Vector3 difference_sign = voxel_difference.Sign();

        int driving_axis = 0;
        int second_axis = 1;
        int third_axis = 2;

        if (voxel_distance.y >= voxel_distance.x && voxel_distance.y >= voxel_distance.z) {
            driving_axis = 1;
            second_axis = 0;
            third_axis = 2;
        }
        else if (voxel_distance.z >= voxel_distance.x && voxel_distance.z >= voxel_distance.y) {
            driving_axis = 2;
            second_axis = 1;
            third_axis = 0;
        }

        int p1 = 2 * voxel_distance[second_axis] - voxel_distance[driving_axis];
        int p2 = 2 * voxel_distance[third_axis] - voxel_distance[driving_axis];

        while (start_voxel[driving_axis] != end_voxel[driving_axis]) {
            start_voxel[driving_axis] += difference_sign[driving_axis];

            if (p1 >= 0) {
                start_voxel[second_axis] += difference_sign[second_axis];
                p1 -= 2 * voxel_distance[driving_axis];
            }
            if (p2 >= 0) {
                start_voxel[third_axis] += difference_sign[third_axis];
                p2 -= 2 * voxel_distance[driving_axis];
            }

            p1 += 2 * voxel_distance[second_axis];
            p2 += 2 * voxel_distance[third_axis];

            int x_index = (int)start_voxel.x;
            int y_index = (int)start_voxel.y;
            int z_index = (int)start_voxel.z;

            x_index = Math::Clip(x_index, 0, (int)space_data.world_size0.x - 1);
            y_index = Math::Clip(y_index, 0, (int)space_data.world_size0.y - 1);
            z_index = Math::Clip(z_index, 0, (int)space_data.world_size0.z - 1);

            unsigned long long world_index = Indexer::FlatIndex3(x_index, y_index, z_index, space_data.world_size0.x, space_data.world_size0.y);

            voxel_data = space_data.space0[world_index];

            hit_position.x = x_index;
            hit_position.y = y_index;
            hit_position.z = z_index;

            Vector3 travel_distance = (end_voxel - start_voxel).Absolute();

            hit_data.position = hit_position;
            hit_data.voxel_data = voxel_data;
            hit_data.distance = Transform::Norm3(voxel_distance) - Transform::Norm3(travel_distance);

            if (voxel_data.entity_id != 0) {
                break;
            }
        }

        return hit_data;
    }

    static __host__ __device__ RaycastHitData Raycast_Old(SpaceData space_data, Vector3 start_position, Vector3 ray_direction, float max_distance) {
        RaycastHitData hit_data{};
        VoxelData voxel_data{};
        Vector3 hit_position{};

        Vector3 query_point = start_position;

        float distance_traveled = 0;
        while (distance_traveled <= max_distance) {
            query_point += ray_direction;

            int x_index = (int)query_point.x;
            int y_index = (int)query_point.y;
            int z_index = (int)query_point.z;

            x_index = Math::Clip(x_index, 0, (int)space_data.world_size0.x - 1);
            y_index = Math::Clip(y_index, 0, (int)space_data.world_size0.y - 1);
            z_index = Math::Clip(z_index, 0, (int)space_data.world_size0.z - 1);

            int world_index = Indexer::FlatIndex3(x_index, y_index, z_index, space_data.world_size0.x, space_data.world_size0.y);

            voxel_data = space_data.space0[world_index];

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