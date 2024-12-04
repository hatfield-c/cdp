#include "CudaIhm.cuh"

//#ifndef __CUDACC__  
//#define __CUDACC__
//#endif

//#include <device_functions.h>

__device__ void CudaIhm::SyncThreads() {
    __syncthreads();
}

__global__ void CudaIhm::SmallestIndexReduction_Kernel(IhmCortex ihm_cortex, int iteration, byte* difference_vector) {
    //__syncthreads();
    void(*func_ptr)() = &SyncThreads;
    ihm_cortex.IndexReduction(iteration, difference_vector, func_ptr);
}

__global__ void CudaIhm::GetDifferenceVector_Kernel (IhmCortex ihm_cortex, byte* phash, byte* difference_vector) {
    ihm_cortex.GetDifferenceVector(phash, difference_vector);
}

__global__ void CudaIhm::GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm) {
    ihm_generator.Generate(space_data, &camera, ihm);
}

unsigned long long CudaIhm::SmallestIndexReduction(IhmCortex ihm_cortex, byte* difference_vector) {
    //int* top_ten_count = new int[10];
    unsigned long long smallest_index = 0;

    dim3 threads_per_block(32, 1, 1);
    unsigned long long units_per_block = threads_per_block.x * ihm_cortex.thread_units;
    unsigned long long block_count = ceil(ihm_cortex.state_count / units_per_block);
    int iterations_max = ceil(log2(ihm_cortex.state_count) / log2(units_per_block));

    dim3 blocks_per_grid(block_count, 1, 1);

    for (int i = 0; i < iterations_max; i++) {
        SmallestIndexReduction_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, i, difference_vector);

        block_count = ceil(block_count / units_per_block);
        blocks_per_grid = dim3(block_count, 1, 1);
    }

    return smallest_index;
}

byte* CudaIhm::GetDifferenceVector(IhmCortex ihm_cortex, byte* phash) {
    unsigned long long memory_size = ihm_cortex.state_count * sizeof(byte);

    byte* difference_vector;
    CudaError::CheckError((cudaError_enum)cudaMalloc(&difference_vector, memory_size), __FILE__, __LINE__);

    dim3 threads_per_block(32, 1, 1);

    unsigned long long block_count = ceil(ihm_cortex.state_count / threads_per_block.x);

    dim3 blocks_per_grid(block_count, 1, 1);

    printf("        Block Count: (%lld, %lld, %lld)\n", blocks_per_grid.x, blocks_per_grid.y, blocks_per_grid.z);
    GetDifferenceVector_Kernel<<<blocks_per_grid, threads_per_block>>>(ihm_cortex, phash, difference_vector);

    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    printf("\n");

    return difference_vector;
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
