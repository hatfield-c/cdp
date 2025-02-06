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
        return 1.0f / 20.0f;
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

        Vector3 current_position = start_position;
        Vector3 end_position = start_position + (ray_direction * max_distance);

        Vector3 start_voxel = start_position.Floor();
        Vector3 current_voxel = start_position.Floor();
        Vector3 end_voxel = end_position.Floor();
        Vector3 end_voxel1 = (end_voxel / space_data.level_stride).Floor();

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
            second_axis = 0;
            third_axis = 1;
        }

        float second_slope = voxel_difference[second_axis] / voxel_difference[driving_axis];
        float third_slope = voxel_difference[third_axis] / voxel_difference[driving_axis];

        float second_bias = start_voxel[second_axis] - (start_voxel[driving_axis] * second_slope);
        float third_bias = start_voxel[third_axis] - (start_voxel[driving_axis] * third_slope);

        //printf("[%.2f %.2f %.2f] [%.2f %.2f %.2f]\n", start_position.x, start_position.y, start_position.z, end_position.x, end_position.y, end_position.z);

        int driving_distance = 0;
        while (driving_distance <= voxel_distance[driving_axis]) {
            driving_distance++;
            current_position[driving_axis] += difference_sign[driving_axis];
            current_position[second_axis] = (current_position[driving_axis] * second_slope) + second_bias;
            current_position[third_axis] = (current_position[driving_axis] * third_slope) + third_bias;

            if (!current_position.IsBounded(Vector::ZERO3(), space_data.world_size0 - 1)) {
                break;
            }

            current_voxel = current_position.Floor();
            Vector3 travel_distance = (current_voxel - start_voxel).Absolute();

            Vector3 query1 = (current_voxel / space_data.level_stride).Floor();

            unsigned long long world_index1 = Indexer::FlatIndex3(query1.x, query1.y, query1.z, space_data.world_size1.x, space_data.world_size1.y);
            VoxelData voxel_data1 = space_data.space1[world_index1];

            if (voxel_data1.entity_id == 0) {
                int voxel_index = floor(current_voxel[driving_axis] / space_data.level_stride);

                if (voxel_index != end_voxel1[driving_axis]) {
                    int remaining_voxels = ((int)current_voxel[driving_axis] % (int)space_data.level_stride);

                    if (difference_sign[driving_axis] > 0) {
                        remaining_voxels = (space_data.level_stride - 1) - remaining_voxels;
                    }
                    
                    driving_distance += remaining_voxels;
                    current_position[driving_axis] += difference_sign[driving_axis] * remaining_voxels;

                    /*printf("    warp: %.2f %d %d %.2f [%.2f %.2f %.2f] [%.2f %.2f %.2f] %d (%.2f %.2f) (%.2f %.2f) %d\n",
                        current_position[driving_axis], driving_distance, remaining_voxels, difference_sign[driving_axis], 
                        query1.x, query1.y, query1.z, 
                        current_voxel.x, current_voxel.y, current_voxel.z,
                        driving_axis, 
                        second_slope, second_bias, 
                        third_slope, third_bias, 
                        ((int)current_voxel[driving_axis] % (int)space_data.level_stride)
                    );*/

                    continue;
                }
            }

            Vector3 query0 = current_voxel;
            unsigned long long world_index0 = Indexer::FlatIndex3(query0.x, query0.y, query0.z, space_data.world_size0.x, space_data.world_size0.y);
            VoxelData voxel_data = space_data.space0[world_index0];

            hit_data.position = current_voxel;
            hit_data.voxel_data = voxel_data;
            hit_data.distance = Transform::Norm3(travel_distance);

            //printf("    %d [%.2f %.2f %.2f] [%.2f %.2f %.2f] %lld\n", voxel_data.entity_id, current_position.x, current_position.y, current_position.z, current_voxel.x, current_voxel.y, current_voxel.z, world_index0);

            if (voxel_data.entity_id != 0) {
                break;
            }
        }

        return hit_data;
    }
};