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
    Vector2 camera_size{ 640, 480 };
    Vector2 phash_texture_size{ 32, 32 };
    Vector2 phash_data_size{ 16, 16 };
    Vector2 fov{ 1.57, 1.29 };
    Vector3 target_offset{ -1, 1, 0 };

    float min_render_distance = 0.05f;
    float max_render_distance = 350.0f;
    float phash_distance = 100.0f;
    unsigned long long camera_pixel_count = 0;
    unsigned long long phash_pixel_count = 0;
    unsigned long long phash_data_count = 0;

    byte* depth_texture;
    byte* phash_texture;
    byte* shaded_texture;
    float* phash_data;
	
	void Init(std::string name, CUdeviceptr depth_texture, CUdeviceptr phash_texture, CUdeviceptr shaded_texture) {
        float pi = 3.141592654f;

		this->name = name;
		this->fov.x = pi / 2;
		this->fov.y = pi / 2;
		this->depth_texture = (byte*)depth_texture;
        this->phash_texture = (byte*)phash_texture;
        this->shaded_texture = (byte*)shaded_texture;
        this->camera_pixel_count = this->camera_size.x * this->camera_size.y;
        this->phash_pixel_count = this->phash_texture_size.x * this->phash_texture_size.y;
        this->phash_data_count = this->phash_data_size.x * this->phash_data_size.y;

        float* phash_data = new float[this->phash_data_count];
        for (int i = 0; i < this->phash_data_count; i++) {
            phash_data[i] = -1;
        }
        int memory_size = this->phash_data_count * sizeof(float);

        CudaError::CheckError((cudaError_enum)cudaMalloc(&this->phash_data, memory_size), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaMemcpy(this->phash_data, phash_data, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);

        delete[] phash_data;
	}

    __device__ void Render(SpaceData space_data) {
        Vector2 pixel_position{
            Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x),
            Indexer::FlatIndex2(threadIdx.y, blockIdx.y, blockDim.y)
        };

        if (pixel_position.x >= this->camera_size.x || pixel_position.y >= this->camera_size.y) {
            return;
        }

        Vector3 ray_direction = this->GetCameraRayDirection(pixel_position);
        RaycastHitData hit_data = Physics::Raycast(space_data, this->transform.position, ray_direction, pixel_position, this->max_render_distance);
        float depth = hit_data.distance;

        float depth_pixel_val = 0;
        if (hit_data.voxel_data.entity_id == 1) {
            depth_pixel_val = depth / this->max_render_distance;
            depth_pixel_val = 255 * (1 - depth_pixel_val);

            if (depth_pixel_val < 0) {
                depth_pixel_val = 0;
            }
        }

        Vector4 depth_color{ depth_pixel_val, depth_pixel_val, depth_pixel_val, 255 };

        this->WriteRGBA(this->depth_texture, pixel_position, this->camera_size, depth_color);
        this->WriteRGBA(this->shaded_texture, pixel_position, this->camera_size, depth_color);
        
        Vector2 spatial_offset = (this->camera_size - 1) / (this->phash_data_size - 1);
        spatial_offset.x = (int)spatial_offset.x;
        spatial_offset.y = (int)spatial_offset.y;

        bool is_phash_pixel = (int)pixel_position.x % (int)spatial_offset.x == 0;
        is_phash_pixel = is_phash_pixel && ((int)pixel_position.y % (int)spatial_offset.y == 0);

        if (is_phash_pixel) {
            Vector2 phash_position = pixel_position / spatial_offset;
            this->WriteFloat(this->phash_data, phash_position, this->phash_data_size, depth);

            Vector4 phash_color{ 255, 255, 255, 255 };
            if (depth > phash_distance) {
                phash_color = Vector4{ 0, 0, 0, 255 };
            }

            Vector2 texture_position{};
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    texture_position.x = (2 * phash_position.x) + i;
                    texture_position.y = (2 * phash_position.y) + j;

                    this->WriteRGBA(this->phash_texture, texture_position, this->phash_texture_size, phash_color);
                }
            }

        }
    }

    __device__ Vector3 GetCameraRayDirection(Vector2 pixel_position) {
        Vector2 fov_offset{
            fov_offset.x = -sinf(this->fov.x / 2),
            fov_offset.y = -sinf(this->fov.y / 2)
        };

        Vector2 screen_interpolation{
            ((2 * pixel_position.x) / this->camera_size.x) - 1,
            ((2 * pixel_position.y) / this->camera_size.y) - 1
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

    __device__ void WriteRGBA(byte* texture, Vector2 pixel_position, Vector2 texture_size, Vector4 rgba) {
        int gpu_index_r = Indexer::FlatIndex3(0, pixel_position.x, pixel_position.y, 4, texture_size.x);
        int gpu_index_g = Indexer::FlatIndex3(1, pixel_position.x, pixel_position.y, 4, texture_size.x);
        int gpu_index_b = Indexer::FlatIndex3(2, pixel_position.x, pixel_position.y, 4, texture_size.x);
        int gpu_index_a = Indexer::FlatIndex3(3, pixel_position.x, pixel_position.y, 4, texture_size.x);

        if (pixel_position.x == 15 && pixel_position.y == 15) {
            //printf("%d\n", gpu_index_r);
        }

        texture[gpu_index_r] = rgba.x;
        texture[gpu_index_g] = rgba.y;
        texture[gpu_index_b] = rgba.z;
        texture[gpu_index_a] = rgba.w;
    }

    __device__ void WriteFloat(float* data_array, Vector2 pixel_position, Vector2 texture_size, float value) {
        int gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        data_array[gpu_index] = value;
    }
};