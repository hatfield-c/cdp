#include "CudaIhm.cuh"


__global__ void CudaIhm::GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm) {
    ihm_generator.Generate(space_data, &camera, ihm);
}

void CudaIhm::GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm) {
    Vector2 resolution = camera.camera_size;

    dim3 threads_per_block(1, 8, 4);

    unsigned long long image_count = ihm_generator.state_count;
    unsigned long long x_blocks = ceil(resolution.x / (float)threads_per_block.y);
    unsigned long long y_blocks = ceil(resolution.y / (float)threads_per_block.z);

    dim3 blocks_per_grid(image_count, x_blocks, y_blocks);

    printf("        Block Count: (%lld, %lld, %lld)\n", image_count, x_blocks, y_blocks);
    printf("        Progress (Max %d *): ", (int)(y_blocks / 6));
    GenerateIhm_Kernel<<<blocks_per_grid, threads_per_block>>>(space_data, camera, ihm_generator, ihm);
    
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    printf("\n");
}
