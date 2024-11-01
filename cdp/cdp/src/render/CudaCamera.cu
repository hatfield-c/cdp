#include "CudaCamera.cuh"

__device__ Vector3 GetCameraRayDirection(CameraData camera_data, Vector2 pixel_position) {
    Vector2 fov_offset{
        fov_offset.x = sinf(camera_data.fov.x / 2),
        fov_offset.y = sinf(camera_data.fov.y / 2)
    };

    Vector2 screen_interpolation{
        screen_interpolation.x = ((2 * pixel_position.x) / camera_data.resolution.x) - 1,
        screen_interpolation.y = ((2 * pixel_position.y) / camera_data.resolution.y) - 1
    };

    Vector3 ray_anchor{
        ray_anchor.x = 1,
        ray_anchor.y = fov_offset.y * screen_interpolation.y,
        ray_anchor.z = fov_offset.x * screen_interpolation.x
    };

    ray_anchor = Quaternion::RotatePoint(ray_anchor, camera_data.transform.rotation);
    Vector3 ray_direction = Transform::Unit3(ray_anchor);

    return ray_direction;
}

__device__ RaycastHitData Raycast(SpaceData space_data, CameraData camera_data, Vector3 ray_direction, Vector2 pixel_position) {
    
    RaycastHitData hit_data{};
    VoxelData voxel_data{};
    Vector3 hit_position{};

    Vector3 query_point{
        camera_data.transform.position.x,
        camera_data.transform.position.y,
        camera_data.transform.position.z
    };
    
    float distance_traveled = 0;
    while (distance_traveled < camera_data.max_render_distance) {
        query_point += ray_direction;

        int x_index = (int)query_point.x;
        int y_index = (int)query_point.y;
        int z_index = (int)query_point.z;

        if (x_index < 0 || y_index < 0 || z_index < 0 || x_index >= space_data.world_size.x || y_index >= space_data.world_size.y || z_index >= space_data.world_size.z) {
            break;
        }

        int world_index = Indexer::FlatIndex3(x_index, y_index, z_index, space_data.world_size.x, space_data.world_size.y, space_data.world_size.z);

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

__device__ void WriteRGBA(byte* image, CameraData camera_data, Vector2 pixel_position, Vector4 rgba) {
    int gpu_index_r = Indexer::FlatIndex3(0, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_g = Indexer::FlatIndex3(1, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_b = Indexer::FlatIndex3(2, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_a = Indexer::FlatIndex3(3, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);

    image[gpu_index_r] = rgba.x;
    image[gpu_index_g] = rgba.y;
    image[gpu_index_b] = rgba.z;
    image[gpu_index_a] = rgba.w;
}

__global__ void RenderCamera_Kernel(CameraData camera_data, SpaceData space_data)
{
    Vector2 pixel_position{
        blockDim.x * blockIdx.x + threadIdx.x,
        blockDim.y * blockIdx.y + threadIdx.y
    };

    if(pixel_position.x >= camera_data.resolution.x || pixel_position.y >= camera_data.resolution.y){
        return;
    }

    byte* image_data = camera_data.gpu_texture;

    Vector3 ray_direction = GetCameraRayDirection(camera_data, pixel_position);    
    RaycastHitData hit_data = Raycast(space_data, camera_data, ray_direction, pixel_position);

    float depth = 0;
    if (hit_data.voxel_data.entity_id == 1) {
        depth = hit_data.distance / camera_data.max_render_distance;
        depth = 255 * (1 - depth);

        if (depth < 0) {
            depth = 0;
        }
    }

    Vector4 depth_color{ depth, depth, depth, 255 };

    WriteRGBA(image_data, camera_data, pixel_position, depth_color);
}

void RenderCamera(CameraData camera_data, WorldSpace* world_space) {
    
    Vector2 resolution = camera_data.resolution;
    
    dim3 threads_per_block(16, 16, 1);

    int x_blocks = ceil(resolution.x / (float)threads_per_block.x);
    int y_blocks = ceil(resolution.y / (float)threads_per_block.y);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);
    
    RenderCamera_Kernel<<<blocks_per_grid, threads_per_block >>>(camera_data, world_space->space_data);

    world_space->CheckCudaError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    world_space->CheckCudaError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}
