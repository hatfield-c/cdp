#include "CudaIhm.cuh"

/*
__global__ void GenerateIhm_Kernel(SpaceData space_data, CameraData camera_data, Vector3 lower, Vector3 upper) {
    Vector2 pixel_position{
        blockDim.x * blockIdx.x + threadIdx.x,
        blockDim.y * blockIdx.y + threadIdx.y
    };

    if (pixel_position.x >= camera_data.resolution.x || pixel_position.y >= camera_data.resolution.y) {
        return;
    }

    byte* image_data = camera_data.gpu_texture;

    Vector3 ray_direction = GetCameraRayDirection(camera_data, pixel_position);
    RaycastHitData hit_data = Physics::Raycast(space_data, camera_data.transform.position, ray_direction, pixel_position, camera_data.max_render_distance);

    float depth = 0;
    if (hit_data.voxel_data.entity_id == 1) {
        depth = hit_data.distance / camera_data.max_render_distance;
        depth = 255 * (1 - depth);

        if (depth < 0) {
            depth = 0;
        }
    }

    Vector4 depth_color{ depth, depth, depth, 255 };

    //WriteRGBA(image_data, camera_data, pixel_position, depth_color);
}

void GenerateIhm(SpaceData space_data, Camera camera, Vector3 lower, Vector3 upper) {
	unsigned long long image_count = space_data.voxel_count * camera_data.ihm_directions.direction_count;

    Vector2 resolution = camera_data.resolution;

    dim3 threads_per_block(8, 4, 1);

    int x_blocks = ceil(resolution.x / (float)threads_per_block.x);
    int y_blocks = ceil(resolution.y / (float)threads_per_block.y);

    dim3 blocks_per_grid(x_blocks, y_blocks, image_count);

    GenerateIhm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera_data, lower, upper);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}
*/