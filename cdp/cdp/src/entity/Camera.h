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
    Vector2 phash_data_stride;
    Vector2 chunk_size;
    Vector2 fov{ 1.309, 1.082 };
    Vector3 target_offset{ -1, 1, 0 };

    float min_render_distance = 0.05f;
    float max_render_distance = 200.0f;
    unsigned long long camera_pixel_count = 0;
    unsigned long long phash_pixel_count = 0;
    unsigned long long phash_data_count = 0;

    byte* depth_texture;
    byte* phash_texture;
    byte* shaded_texture;
    RaycastHitData* depth_data;
    byte* phash_data;
	
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
        this->phash_data_stride = (this->camera_size / this->phash_data_size).Ceil();
        this->chunk_size = (this->camera_size / this->phash_data_size).Ceil();

        //RaycastHitData* depth_data = new RaycastHitData[this->camera_pixel_count];
        byte* phash_data = new byte[this->camera_pixel_count];

        int depth_memory_size = this->camera_pixel_count * sizeof(RaycastHitData);
        int phash_memory_size = this->phash_data_count * sizeof(byte);

        //CudaError::CheckError((cudaError_enum)cudaMalloc(&this->depth_data, depth_memory_size), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaMalloc(&this->phash_data, depth_memory_size), __FILE__, __LINE__);
        //CudaError::CheckError((cudaError_enum)cudaMemcpy(this->depth_data, depth_data, depth_memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaMemcpy(this->phash_data, phash_data, phash_memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
	}

    byte* GetPhash() {
        byte* phash_cpu = new byte[this->phash_data_count];
        int phash_memory_size = this->phash_data_count * sizeof(byte);

        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaMemcpy(phash_cpu, this->phash_data, phash_memory_size, cudaMemcpyDeviceToHost), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

        return phash_cpu;
    }

    __device__ void Render(SpaceData space_data) {
        Vector2 phash_position{ threadIdx.x, threadIdx.y };
        Vector2 pixel_position;

        float avg_distance = 0;
        int avg_count = 0;

        for (int i = 0; i < this->chunk_size.x; i++) {
            pixel_position.x = Indexer::FlatIndex2(i, phash_position.x, this->chunk_size.x);

            if (pixel_position.x >= this->camera_size.x) {
                continue;
            }

            for (int j = 0; j < this->chunk_size.y; j++) {
                pixel_position.y = Indexer::FlatIndex2(j, phash_position.y, this->chunk_size.y);

                if (pixel_position.y >= this->camera_size.y) {
                    continue;
                }

                Vector3 ray_direction = Camera::GetCameraRayDirection(pixel_position, this->camera_size, this->fov, this->transform.rotation);
                RaycastHitData hit_data = Physics::Raycast(space_data, this->transform.position, ray_direction, pixel_position, this->max_render_distance);
                float depth = hit_data.distance;

                byte depth_pixel_val = this->DepthToInversePixel(depth);
                Vector4 depth_color{ depth_pixel_val, depth_pixel_val, depth_pixel_val, 255 };

                this->WriteRGBA(this->depth_texture, pixel_position, this->camera_size, depth_color);
                this->WriteRGBA(this->shaded_texture, pixel_position, this->camera_size, depth_color);

                if (depth >= this->max_render_distance) {
                    continue;
                }

                avg_distance += depth;
                avg_count++;
            }
        }

        if (avg_count < 1) {
            avg_distance = this->max_render_distance;
            avg_count = 1;
        }

        avg_distance = avg_distance / avg_count;

        byte phash_pixel_value = this->DepthToPixel(avg_distance);
        Vector4 phash_color{ phash_pixel_value, phash_pixel_value, phash_pixel_value, 255 };

        Camera::WriteByte(this->phash_data, phash_position, this->phash_data_size, phash_pixel_value);

        Vector2 texture_position{};
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                texture_position.x = (2 * phash_position.x) + i;
                texture_position.y = (2 * phash_position.y) + j;

                Camera::WriteRGBA(this->phash_texture, texture_position, this->phash_texture_size, phash_color);
            }
        }
    }

    /*__device__ void DepthUpdate(SpaceData space_data) {
        Vector2 pixel_position{
            Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x),
            Indexer::FlatIndex2(threadIdx.y, blockIdx.y, blockDim.y)
        };

        if (pixel_position.x >= this->camera_size.x || pixel_position.y >= this->camera_size.y) {
            return;
        }

        Vector3 ray_direction = this->GetCameraRayDirection(pixel_position, this->camera_size, this->fov, this->transform.rotation);
        RaycastHitData hit_data = Physics::Raycast(space_data, this->transform.position, ray_direction, pixel_position, this->max_render_distance);
        
        this->WriteDepth(this->depth_data, pixel_position, this->camera_size, hit_data);
    }

    __device__ void Render(SpaceData space_data) {
        Vector2 pixel_position{
            Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x),
            Indexer::FlatIndex2(threadIdx.y, blockIdx.y, blockDim.y)
        };

        RaycastHitData raycast_data = this->ReadDepth(this->depth_data, pixel_position, this->camera_size);
        float depth = raycast_data.distance;

        float depth_pixel_val = 0;
        if (raycast_data.voxel_data.entity_id == 1) {
            depth_pixel_val = depth / this->max_render_distance;
            depth_pixel_val = 255 * (1 - depth_pixel_val);

            if (depth_pixel_val < 0) {
                depth_pixel_val = 0;
            }
        }
        
        Vector4 depth_color{depth_pixel_val, depth_pixel_val, depth_pixel_val, 255};

        this->WriteRGBA(this->depth_texture, pixel_position, this->camera_size, depth_color);
        this->WriteRGBA(this->shaded_texture, pixel_position, this->camera_size, depth_color);

        Vector2 spatial_offset = (this->camera_size - 1) / (this->phash_data_size - 1);
        spatial_offset.x = (int)spatial_offset.x;
        spatial_offset.y = (int)spatial_offset.y;

        bool is_phash_pixel = (int)pixel_position.x % (int)spatial_offset.x == 0;
        is_phash_pixel = is_phash_pixel && ((int)pixel_position.y % (int)spatial_offset.y == 0);

        if (is_phash_pixel) {
            this->Phash(space_data, raycast_data, pixel_position, spatial_offset);
        } 
    }*/

    static __device__ Vector3 GetCameraRayDirection(Vector2 pixel_position, Vector2 canvas_size, Vector2 fov, Vector4 camera_rotation) {
        Vector2 fov_offset{
            fov_offset.x = -sinf(fov.x / 2),
            fov_offset.y = -sinf(fov.y / 2)
        };

        Vector2 screen_interpolation{
            ((2 * pixel_position.x) / canvas_size.x) - 1,
            ((2 * pixel_position.y) / canvas_size.y) - 1
        };

        Vector3 ray_anchor{
            1,
            fov_offset.y * screen_interpolation.y,
            fov_offset.x * screen_interpolation.x
        };

        ray_anchor = Quaternion::RotatePoint(ray_anchor, camera_rotation);

        Vector3 ray_direction = Transform::Unit3(ray_anchor);

        return ray_direction;
    }

    /*__device__ void Phash(SpaceData space_data, RaycastHitData raycast_data, Vector2 pixel_position, Vector2 spatial_offset) {
        Vector2 phash_position = pixel_position / spatial_offset;

        float avg_distance = 0;
        int avg_count = 0;
        for (int i = 0; i < this->phash_data_stride.x; i++) {
            for (int j = 0; j < this->phash_data_stride.y; j++) {
                Vector2 pixel_offset{ i, j };
                pixel_offset -= (this->phash_data_stride / 2).Floor();
                pixel_offset += pixel_position;

                if (pixel_offset.x < 0 || pixel_offset.y < 0 || pixel_offset.x >= this->camera_size.x || pixel_offset.y >= this->camera_size.y) {
                    continue;
                }

                RaycastHitData data = this->ReadDepth(this->depth_data, pixel_offset, this->camera_size);
                float depth = data.distance;

                if (depth >= this->max_render_distance) {
                    continue;
                }

                avg_distance += depth;
                avg_count++;
            }
        }

        if (avg_count < 1) {
            avg_distance = this->max_render_distance;
            avg_count = 1;
        }

        avg_distance = avg_distance / avg_count;

        float distance = avg_distance / 10;
        //float distance = raycast_data.distance / 10;
        distance = Transform::Clip(distance, 0.0, 20.0);
        byte value = (byte)(int)(255 * distance / 20.0);
        Vector4 phash_color{ value, value, value, 255 };

        Camera::WriteByte(this->phash_data, phash_position, this->phash_data_size, value);

        Vector2 texture_position{};
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                texture_position.x = (2 * phash_position.x) + i;
                texture_position.y = (2 * phash_position.y) + j;

                Camera::WriteRGBA(this->phash_texture, texture_position, this->phash_texture_size, phash_color);
            }
        }
    }*/

    __host__ __device__ byte DepthToPixel(float depth) {
        float depth_pixel_val = Transform::Clip(depth, 0.0, this->max_render_distance);
        depth_pixel_val = depth_pixel_val / this->max_render_distance;
        depth_pixel_val = 255 * depth_pixel_val;
        depth_pixel_val = Transform::Clip(depth_pixel_val, 0.0, 255.0);

        byte pixel_val = depth_pixel_val;

        return pixel_val;
    }

    __host__ __device__ byte DepthToInversePixel(float depth) {
        float depth_pixel_val = Transform::Clip(depth, 0.0, this->max_render_distance);
        depth_pixel_val = depth_pixel_val / this->max_render_distance;
        depth_pixel_val = 255 * (1 - depth_pixel_val);
        depth_pixel_val = Transform::Clip(depth_pixel_val, 0.0, 255.0);

        byte pixel_val = depth_pixel_val;

        return pixel_val;
    }

    static __device__ void WriteRGBA(byte* texture, Vector2 pixel_position, Vector2 texture_size, Vector4 rgba) {
        unsigned long long gpu_index_r = Indexer::FlatIndex3(0, pixel_position.x, pixel_position.y, 4, texture_size.x);
        unsigned long long gpu_index_g = Indexer::FlatIndex3(1, pixel_position.x, pixel_position.y, 4, texture_size.x);
        unsigned long long gpu_index_b = Indexer::FlatIndex3(2, pixel_position.x, pixel_position.y, 4, texture_size.x);
        unsigned long long gpu_index_a = Indexer::FlatIndex3(3, pixel_position.x, pixel_position.y, 4, texture_size.x);

        texture[gpu_index_r] = rgba.x;
        texture[gpu_index_g] = rgba.y;
        texture[gpu_index_b] = rgba.z;
        texture[gpu_index_a] = rgba.w;
    }

    static __device__ RaycastHitData ReadDepth(RaycastHitData* data_array, Vector2 pixel_position, Vector2 texture_size) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        return data_array[gpu_index];
    }

    static __device__ void WriteDepth(RaycastHitData* data_array, Vector2 pixel_position, Vector2 texture_size, RaycastHitData depth_data) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        data_array[gpu_index] = depth_data;
    }

    static __device__ void WriteFloat(float* data_array, Vector2 pixel_position, Vector2 texture_size, float value) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        data_array[gpu_index] = value;
    }

    static __device__ void WriteByte(byte* data_array, Vector2 pixel_position, Vector2 texture_size, byte value) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        data_array[gpu_index] = value;
    }
};