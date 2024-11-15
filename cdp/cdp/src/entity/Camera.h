#pragma once

#include <string>

#include "cuda.h"

#include "../engine/WorldSpace.h"
#include "../engine/Physics.h"
#include "../engine/Transform.h"
#include "../engine/Quaternion.h"
#include "../engine/Indexer.h"

typedef unsigned char byte;

struct Camera {
    std::string name;
    Transform transform{};
    Vector2 resolution{ 640, 480 };
    Vector2 fov{ 1.57, 1.29 };
    Vector3 target_offset{ -1, 1, 0 };

    float min_render_distance = 0.05f;
    float max_render_distance = 350.0f;

    byte* depth_texture;
    byte* phash_texture;
    byte* shaded_texture;
	
	void Init(std::string name, CUdeviceptr depth_texture, CUdeviceptr phash_texture, CUdeviceptr shaded_texture) {
        float pi = 3.141592654f;

		this->name = name;
		this->fov.x = pi / 2;
		this->fov.y = pi / 2;
		this->depth_texture = (byte*)depth_texture;
        this->phash_texture = (byte*)phash_texture;
        this->shaded_texture = (byte*)shaded_texture;
	}

    __device__ void Render(SpaceData space_data) {
        Vector2 pixel_position{
            blockDim.x * blockIdx.x + threadIdx.x,
            blockDim.y * blockIdx.y + threadIdx.y
        };

        if (pixel_position.x >= this->resolution.x || pixel_position.y >= this->resolution.y) {
            return;
        }

        Vector3 ray_direction = this->GetCameraRayDirection(pixel_position);
        RaycastHitData hit_data = Physics::Raycast(space_data, this->transform.position, ray_direction, pixel_position, this->max_render_distance);

        float depth = 0;
        if (hit_data.voxel_data.entity_id == 1) {
            depth = hit_data.distance / this->max_render_distance;
            depth = 255 * (1 - depth);

            if (depth < 0) {
                depth = 0;
            }
        }

        Vector4 depth_color{ depth, depth, depth, 255 };

        this->WriteRGBA(pixel_position, depth_color);
    }

    __device__ Vector3 GetCameraRayDirection(Vector2 pixel_position) {
        Vector2 fov_offset{
            fov_offset.x = -sinf(this->fov.x / 2),
            fov_offset.y = -sinf(this->fov.y / 2)
        };

        Vector2 screen_interpolation{
            ((2 * pixel_position.x) / this->resolution.x) - 1,
            ((2 * pixel_position.y) / this->resolution.y) - 1
        };

        Vector3 ray_anchor{
            1,
            fov_offset.y * screen_interpolation.y,
            fov_offset.x * screen_interpolation.x
        };

        ray_anchor = Quaternion::RotatePoint(ray_anchor, this->transform.rotation);

        Vector3 ray_direction = Transform::Unit3(ray_anchor);

        return ray_direction;
    }

    __device__ void WriteRGBA(Vector2 pixel_position, Vector4 rgba) {
        int gpu_index_r = Indexer::FlatIndex3(0, pixel_position.x, pixel_position.y, 4, this->resolution.x);
        int gpu_index_g = Indexer::FlatIndex3(1, pixel_position.x, pixel_position.y, 4, this->resolution.x);
        int gpu_index_b = Indexer::FlatIndex3(2, pixel_position.x, pixel_position.y, 4, this->resolution.x);
        int gpu_index_a = Indexer::FlatIndex3(3, pixel_position.x, pixel_position.y, 4, this->resolution.x);

        this->depth_texture[gpu_index_r] = rgba.x;
        this->depth_texture[gpu_index_g] = rgba.y;
        this->depth_texture[gpu_index_b] = rgba.z;
        this->depth_texture[gpu_index_a] = rgba.w;
    }
};