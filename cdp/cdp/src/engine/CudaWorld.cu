#include "CudaWorld.cuh"

__global__ void AssignChunk_Kernel(SpaceData space_data, VoxelData voxel_data, Vector3 lower, Vector3 upper) {
    Vector3 local_offset{
        blockDim.x * blockIdx.x + threadIdx.x,
        blockDim.y * blockIdx.y + threadIdx.y,
        blockDim.z * blockIdx.z + threadIdx.z
    };

    if (local_offset.x >= upper.x || local_offset.y >= upper.y || local_offset.z >= upper.z) {
        return;
    }

    Vector3 voxel_indices = local_offset + lower;

    if (voxel_indices.x >= space_data.world_size.x || voxel_indices.y >= space_data.world_size.y || voxel_indices.z >= space_data.world_size.z) {
        return;
    }

    int space_index = Indexer::FlatIndex3(voxel_indices.x, voxel_indices.y, voxel_indices.z, space_data.world_size.x, space_data.world_size.y, space_data.world_size.z);

    space_data.space_cuda[space_index] = voxel_data;
}

__global__ void AssignPoints_Kernel(SpaceData space_data, Vector3* points, int point_count, VoxelData voxel_data) {
    int point_index = blockDim.x * blockIdx.x + threadIdx.x;

    if (point_index >= point_count) {
        return;
    }
    
    Vector3 point = points[point_index];

    if (point.x >= space_data.world_size.x || point.y >= space_data.world_size.y || point.z >= space_data.world_size.z) {
        return;
    }

    int space_index = Indexer::FlatIndex3(point.x, point.y, point.z, space_data.world_size.x, space_data.world_size.y, space_data.world_size.z);

    space_data.space_cuda[space_index] = voxel_data;
}

void AssignChunk(SpaceData space_data, VoxelData voxel_data, Vector3 lower, Vector3 upper) {
    float x_size = upper.x - lower.x;
    float y_size = upper.y - lower.y;
    float z_size = upper.z - lower.z;

    dim3 threads_per_block(4, 4, 4);

    int x_blocks = ceil(x_size / (float)threads_per_block.x);
    int y_blocks = ceil(y_size / (float)threads_per_block.y);
    int z_blocks = ceil(z_size / (float)threads_per_block.z);

    dim3 blocks_per_grid(x_blocks, y_blocks, z_blocks);

	AssignChunk_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, voxel_data, lower, upper);
}

void AssignAll(SpaceData space_data, VoxelData voxel_data) {
	Vector3 lower{ 0, 0, 0 };
	Vector3 upper{ space_data.world_size.x, space_data.world_size.y, space_data.world_size.z };

	AssignChunk(space_data, voxel_data, lower, upper);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void AssignPoints(SpaceData space_data, Vector3* points, int point_count, VoxelData voxel_data) {
    dim3 threads_per_block(32, 1, 1);

    int x_blocks = ceil(point_count / (float)threads_per_block.x);

    dim3 blocks_per_grid(x_blocks, 1, 1);

    AssignPoints_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, points, point_count, voxel_data);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}