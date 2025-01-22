#include "CudaWorld.cuh"

__global__ void CudaWorld::FillBox_Kernel(SpaceBuilder space_builder, SpaceData space_data, VoxelData voxel_data, Vector3 origin, Vector3 width) {
    space_builder.FillBox(space_data, voxel_data, origin, width);
}

__global__ void CudaWorld::WritePoints_Kernel(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count, VoxelData voxel_data) {
    space_builder.WritePoints(space_data, points, point_count, voxel_data);
}

__global__ void CudaWorld::PlanarDensify_Kernel(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count) {
    space_builder.PlanarDensify(space_data, points, point_count);
}

__global__ void CudaWorld::StochasticSubtraction_Kernel(SpaceBuilder space_builder, SpaceData space_data) {
    space_builder.StochasticSubtraction(space_data);
}

VoxelData CudaWorld::ReadVoxel(SpaceData space_data, Vector3 position) {
    unsigned long long voxel_index = Indexer::FlatIndex3(position.x, position.y, position.z, space_data.world_size0.x, space_data.world_size0.y);
    VoxelData voxel_data{};

    CudaError::CheckError((cudaError_enum)cudaMemcpy(&voxel_data, space_data.space0 + voxel_index , sizeof(VoxelData), cudaMemcpyDeviceToHost), __FILE__, __LINE__);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    return voxel_data;
}

void CudaWorld::FillBox(SpaceBuilder space_builder, SpaceData space_data, VoxelData voxel_data, Vector3 lower, Vector3 width) {
    dim3 threads_per_block(4, 4, 4);

    int x_blocks = ceil(width.x / (float)threads_per_block.x);
    int y_blocks = ceil(width.y / (float)threads_per_block.y);
    int z_blocks = ceil(width.z / (float)threads_per_block.z);

    dim3 blocks_per_grid(x_blocks, y_blocks, z_blocks);

    CudaWorld::FillBox_Kernel<<<blocks_per_grid, threads_per_block>>>(space_builder, space_data, voxel_data, lower, width);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void CudaWorld::AssignAll(SpaceBuilder space_builder, SpaceData space_data, VoxelData voxel_data) {
	Vector3 lower{ 0, 0, 0 };
	Vector3 upper{ space_data.world_size0.x, space_data.world_size0.y, space_data.world_size0.z };

    CudaWorld::FillBox(space_builder, space_data, voxel_data, lower, upper);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void CudaWorld::WritePoints(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count, VoxelData voxel_data) {
    dim3 threads_per_block(32, 1, 1);

    int x_blocks = ceil(point_count / (float)threads_per_block.x);

    dim3 blocks_per_grid(x_blocks, 1, 1);

    CudaWorld::WritePoints_Kernel<<<blocks_per_grid, threads_per_block>>>(space_builder, space_data, points, point_count, voxel_data);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void CudaWorld::PlanarDensify(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count) {
    dim3 threads_per_block(512, 1, 1);

    unsigned long long x_blocks = ceil(point_count / threads_per_block.x);

    dim3 blocks_per_grid(x_blocks, 1, 1);

    CudaWorld::PlanarDensify_Kernel<<<blocks_per_grid, threads_per_block>>>(space_builder, space_data, points, point_count);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

void CudaWorld::StochasticSubtraction(SpaceBuilder space_builder, SpaceData space_data) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    dim3 threads_per_block(32, 1, 1);

    int x_blocks = ceil(space_data.voxel_count0 / threads_per_block.x);

    dim3 blocks_per_grid(x_blocks, 1, 1);

    printf("    Stochastic Subtraction:\n");
    printf("        Block Count : (%lld, %lld, %lld)\n", x_blocks, 1, 1);
    printf("        Thread Count: (%lld, %lld, %lld)\n", threads_per_block.x, 1, 1);
    printf("        Progress (Max 20 *): ");
    CudaWorld::StochasticSubtraction_Kernel<<<blocks_per_grid, threads_per_block>>>(space_builder, space_data);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    printf("\n        Done!\n");
    printf("            Time Elapsed: %d s\n", time_lapsed);
}