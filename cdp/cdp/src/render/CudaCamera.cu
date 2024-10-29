#include "CudaCamera.cuh"

__device__ float Magnitude3(Transform::Vector3 vector) {
    float magnitude = 0.0f;
    magnitude += vector.x * vector.x;
    magnitude += vector.y * vector.y;
    magnitude += vector.z * vector.z;

    return sqrt(magnitude);
}

__device__ float Magnitude4(Transform::Vector4 vector) {
    float magnitude = 0.0f;
    magnitude += vector.x * vector.x;
    magnitude += vector.y * vector.y;
    magnitude += vector.z * vector.z;
    magnitude += vector.w * vector.w;

    return sqrt(magnitude);
}

__device__ Transform::Vector3 GetCameraRayDirection(CameraData camera_data, Transform::Vector2 pixel_position) {
    Transform::Vector2 fov_offset{};
    fov_offset.x = sinf(camera_data.fov.x / 2);
    fov_offset.y = sinf(camera_data.fov.y / 2);

    Transform::Vector2 screen_interpolation{};
    screen_interpolation.x = ((2 * pixel_position.x) / camera_data.resolution.x) - 1;
    screen_interpolation.y = ((2 * pixel_position.y) / camera_data.resolution.y) - 1;

    Transform::Vector3 ray_anchor{};
    ray_anchor.x = 1;
    ray_anchor.y = fov_offset.y * screen_interpolation.y;
    ray_anchor.z = fov_offset.x * screen_interpolation.x;

    ray_anchor = RotatePoint(ray_anchor, camera_data.transform.rotation);

    Transform::Vector3 ray_direction{};
    float anchor_magnitude = Magnitude3(ray_anchor);
    ray_direction.x = ray_anchor.x / anchor_magnitude;
    ray_direction.y = ray_anchor.y / anchor_magnitude;
    ray_direction.z = ray_anchor.z / anchor_magnitude;

    return ray_direction;
}
__device__ Transform::Vector4 GetQuaternionConjugate(Transform::Vector4 original) {
    Transform::Vector4 conjugate{};
    conjugate.x = -original.x;
    conjugate.y = -original.y;
    conjugate.z = -original.z;
    conjugate.w = original.w;

    return conjugate;
}

__device__ Transform::Vector4 MultiplyQuaternions(Transform::Vector4 q0, Transform::Vector4 q1, bool is_normalized) {
    Transform::Vector4 result{};
    
    result.w = (q0.w * q1.w) - (q0.x * q1.x) - (q0.y * q1.y) - (q0.z * q1.z);
    result.x = (q0.w * q1.x) + (q0.x * q1.w) + (q0.y * q1.z) - (q0.z * q1.y);
    result.y = (q0.w * q1.y) - (q0.x * q1.z) + (q0.y * q1.w) + (q0.z * q1.x);
    result.z = (q0.w * q1.z) + (q0.x * q1.y) - (q0.y * q1.x) + (q0.z * q1.w);

    if(is_normalized) {
        float magnitude = Magnitude4(result);

        result.w = result.w / magnitude;
        result.x = result.x / magnitude;
        result.y = result.y / magnitude;
        result.z = result.z / magnitude;
    }

    return result;
}

__device__ Transform::Vector3 RotatePoint(Transform::Vector3 position, Transform::Vector4 quaternion) {
    Transform::Vector4 position_quaternized{};
    position_quaternized.x = position.x;
    position_quaternized.y = position.y;
    position_quaternized.z = position.z;
    position_quaternized.w = 0;

    Transform::Vector4 conjugate = GetQuaternionConjugate(quaternion);
    Transform::Vector4 rotated_points = MultiplyQuaternions(quaternion, position_quaternized, false);
    rotated_points = MultiplyQuaternions(rotated_points, conjugate, false);

    Transform::Vector3 result{};
    result.x = rotated_points.x;
    result.y = rotated_points.y;
    result.z = rotated_points.z;

    return result;
}

__device__ RaycastHitData Raycast(SpaceData space_data, CameraData camera_data, Transform::Vector3 ray_direction, Transform::Vector2 pixel_position) {
    
    RaycastHitData hit_data{};
    VoxelData voxel_data{};
    Transform::Vector3 hit_position{};

    Transform::Vector3 query_point{};
    query_point.x = camera_data.transform.position.x;
    query_point.y = camera_data.transform.position.y;
    query_point.z = camera_data.transform.position.z;
    
    float distance_traveled = 0;
    while (distance_traveled < camera_data.max_render_distance) {
        query_point.x += ray_direction.x;
        query_point.y += ray_direction.y;
        query_point.z += ray_direction.z;

        int x_index = (int)query_point.x;
        int y_index = (int)query_point.y;
        int z_index = (int)query_point.z;

        if (x_index < 0 || y_index < 0 || z_index < 0 || x_index >= space_data.world_size.x || y_index >= space_data.world_size.y || z_index >= space_data.world_size.z) {
            break;
        }

        int world_index = GetIndexCWH(x_index, y_index, z_index, space_data.world_size.x, space_data.world_size.y, space_data.world_size.z);

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

__device__ int GetIndexCWH(int c, int w, int h, int c_max, int w_max, int h_max) {
    int index = c + (w * c_max) + (h * c_max * w_max);

    return index;
}

__device__ void WriteRGBA(byte* image, CameraData camera_data, Transform::Vector2 pixel_position, Transform::Vector4 rgba) {
    int gpu_index_r = GetIndexCWH(0, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_g = GetIndexCWH(1, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_b = GetIndexCWH(2, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);
    int gpu_index_a = GetIndexCWH(3, pixel_position.x, pixel_position.y, 4, camera_data.resolution.x, camera_data.resolution.y);

    image[gpu_index_r] = rgba.x;
    image[gpu_index_g] = rgba.y;
    image[gpu_index_b] = rgba.z;
    image[gpu_index_a] = rgba.w;

}

__global__ void RenderCamera_Kernel(CameraData camera_data, SpaceData space_data)
{
    Transform::Vector2 pixel_position{};
    pixel_position.x = blockDim.x * blockIdx.x + threadIdx.x;
    pixel_position.y = blockDim.y * blockIdx.y + threadIdx.y;

    if(pixel_position.x >= camera_data.resolution.x || pixel_position.y >= camera_data.resolution.y){
        return;
    }

    byte* image_data = camera_data.gpu_texture;
    Transform::Vector4 red_color{ 255, 0, 0, 255 };
    Transform::Vector4 blue_color{ 0, 0, 255, 255 };

    Transform::Vector3 ray_direction = GetCameraRayDirection(camera_data, pixel_position);
    
    RaycastHitData hit_data = Raycast(space_data, camera_data, ray_direction, pixel_position);
    float depth = 0;
    
    if (hit_data.voxel_data.entity_id == 1) {
        depth = hit_data.distance / camera_data.max_render_distance;
        depth = 255 * (1 - depth);

        if (depth < 0) {
            depth = 0;
        }
    }

    Transform::Vector4 depth_color{ depth, depth, depth, 255 };

    WriteRGBA(image_data, camera_data, pixel_position, depth_color);
}

void RenderCamera(CameraData camera_data, WorldSpace* world_space) {
    
    Transform::Vector2 resolution = camera_data.resolution;
    
    dim3 threads_per_block(16, 16, 1);

    int x_blocks = ceil(resolution.x / (float)threads_per_block.x);
    int y_blocks = ceil(resolution.y / (float)threads_per_block.y);

    dim3 blocks_per_grid(x_blocks, y_blocks, 1);
    
    RenderCamera_Kernel<<<blocks_per_grid, threads_per_block >>>(camera_data, world_space->space_data);

    world_space->CheckCudaError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
}
