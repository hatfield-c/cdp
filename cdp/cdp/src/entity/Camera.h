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
    Vector2 box_filter_stride{ 8, 6 };
    Vector2 chunk_size;
    Vector2 fov{ 1.309, 1.082 };
    Vector3 target_offset{ -1, 1, 0 };

    float min_render_distance = 0.05f;
    float max_render_distance = 200.0f;
    float max_distance = 20.0f;
    unsigned long long camera_pixel_count = 0;
    unsigned long long phash_pixel_count = 0;
    unsigned long long phash_data_count = 0;

    byte* depth_texture;
    byte* phash_texture;
    byte* centroid_texture;
    RaycastHitData* depth_data;
    byte* phash_data;
    Vector3* render_cloud;
	
	void Init(std::string name, CUdeviceptr depth_texture, CUdeviceptr phash_texture, CUdeviceptr centroid_texture) {
        float pi = 3.141592654f;

		this->name = name;
		this->fov.x = pi / 2;
		this->fov.y = pi / 2;
		this->depth_texture = (byte*)depth_texture;
        this->phash_texture = (byte*)phash_texture;
        this->centroid_texture = (byte*)centroid_texture;
        this->camera_pixel_count = this->camera_size.x * this->camera_size.y;
        this->phash_pixel_count = this->phash_texture_size.x * this->phash_texture_size.y;
        this->phash_data_count = this->phash_data_size.x * this->phash_data_size.y;
        this->phash_data_stride = (this->camera_size / this->phash_data_size).Ceil();
        this->chunk_size = (this->camera_size / this->phash_data_size).Ceil();

        int phash_memory_size = this->phash_data_count * sizeof(byte);
        int cloud_memory_size = this->phash_data_count * sizeof(Vector3);

        CudaError::CheckError((cudaError_enum)cudaMalloc(&this->phash_data, phash_memory_size), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaMalloc(&this->render_cloud, cloud_memory_size), __FILE__, __LINE__);
        cudaMemset(this->phash_data, 0, phash_memory_size);
        cudaMemset(this->render_cloud, 0, cloud_memory_size);

        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
	}

    byte* GetPhash() {
        byte* phash_cpu = new byte[this->phash_data_count];
        int phash_memory_size = this->phash_data_count * sizeof(byte);

        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaMemcpy(phash_cpu, this->phash_data, phash_memory_size, cudaMemcpyDeviceToHost), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

        return phash_cpu;
    }

    Vector3* GetCloud() {
        Vector3* render_cloud = new Vector3[this->phash_data_count];
        int phash_memory_size = this->phash_data_count * sizeof(Vector3);

        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaMemcpy(render_cloud, this->render_cloud, phash_memory_size, cudaMemcpyDeviceToHost), __FILE__, __LINE__);
        CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

        return render_cloud;
    }

    __device__ void Render(SpaceData space_data) {
        Vector2 phash_position{ 
            threadIdx.x, 
            Indexer::FlatIndex2(threadIdx.y, blockIdx.y, blockDim.y)
        };
        Vector2 pixel_position;

        if (!phash_position.IsBounded(Vector::ZERO2(), this->phash_data_size - 1)) {
            return;
        }

        float avg_distance = 0;
        int avg_count = 0;

        /// debug
        //if (phash_position.x != 7 || phash_position.y != 7) {
            //return;
        //}

        for (int i = 0; i < this->chunk_size.x; i += this->box_filter_stride.x) {
            pixel_position.x = Indexer::FlatIndex2(i, phash_position.x, this->chunk_size.x);

            if (pixel_position.x >= this->camera_size.x) {
                continue;
            }

            for (int j = 0; j < this->chunk_size.y; j+= this->box_filter_stride.y) {
                pixel_position.y = Indexer::FlatIndex2(j, phash_position.y, this->chunk_size.y);

                if (pixel_position.y >= this->camera_size.y) {
                    continue;
                }

                /// debug
                //if (i != 0 || j != 0) {
                    //return;
                //}

                Vector3 ray_direction = Camera::GetCameraRayDirection(pixel_position, this->camera_size, this->fov, this->transform.rotation);
                RaycastHitData hit_data = Physics::Raycast(space_data, this->transform.position, ray_direction, this->max_render_distance);
                float depth = hit_data.distance;

                if (hit_data.voxel_data.entity_id == 0) {
                    depth = this->max_render_distance;
                }

                byte depth_pixel_val = this->DepthToInversePixel(depth);
                Vector4 depth_color{ depth_pixel_val, depth_pixel_val, depth_pixel_val, 255 };

                for (int w = 0; w < this->box_filter_stride.x; w++) {
                    for (int h = 0; h < this->box_filter_stride.x; h++) {
                        this->WriteRGBA(this->depth_texture, pixel_position + Vector2{ (float)w, (float)h }, this->camera_size, depth_color);
                    }
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
                Camera::WriteRGBA(this->centroid_texture, texture_position, this->phash_texture_size, phash_color);
            }
        }
    }

    __device__ void BuildCloud() {
        Vector2 phash_position{
            threadIdx.x,
            Indexer::FlatIndex2(threadIdx.y, blockIdx.y, blockDim.y)
        };

        byte phash_val = this->ReadByte(this->phash_data, phash_position, this->phash_data_size);
        
        Vector3 ray_direction = Camera::GetCameraRayDirection(phash_position, this->phash_data_size, this->fov, this->transform.rotation);
        float depth = (phash_val / 256.0) * this->max_distance;

        this->WriteVector3(this->render_cloud, phash_position, this->phash_data_size, ray_direction * depth);
    }

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

    __host__ __device__ byte DepthToPixel(float depth) {
        float depth_pixel_val = Math::Clip(depth, 0.0, this->max_render_distance);
        depth_pixel_val = depth_pixel_val / this->max_render_distance;
        depth_pixel_val = 255 * depth_pixel_val;
        depth_pixel_val = Math::Clip(depth_pixel_val, 0.0, 255.0);

        byte pixel_val = depth_pixel_val;

        return pixel_val;
    }

    __host__ __device__ byte DepthToInversePixel(float depth) {
        float depth_pixel_val = Math::Clip(depth, 0.0, this->max_render_distance);
        depth_pixel_val = depth_pixel_val / this->max_render_distance;
        depth_pixel_val = 255 * (1 - depth_pixel_val);
        depth_pixel_val = Math::Clip(depth_pixel_val, 0.0, 255.0);

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

    static __device__ byte ReadByte(byte* data_array, Vector2 pixel_position, Vector2 texture_size) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        return data_array[gpu_index];
    }

    static __device__ void WriteByte(byte* data_array, Vector2 pixel_position, Vector2 texture_size, byte value) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        data_array[gpu_index] = value;
    }

    static __device__ void WriteFloat(float* data_array, Vector2 pixel_position, Vector2 texture_size, float value) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        data_array[gpu_index] = value;
    }

    static __device__ void WriteVector3(Vector3* data_array, Vector2 pixel_position, Vector2 texture_size, Vector3 value) {
        unsigned long long gpu_index = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, texture_size.x);

        data_array[gpu_index] = value;
    }
};