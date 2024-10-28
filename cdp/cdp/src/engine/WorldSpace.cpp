#include "WorldSpace.h"

#include <iostream>

WorldSpace::WorldSpace() {
	this->space_data.voxel_count = (unsigned long long)(this->space_data.world_size.x * this->space_data.world_size.y * this->space_data.world_size.z);
	this->space_data.memory_size = this->space_data.voxel_count * sizeof(VoxelData);

	this->space_data.space = (VoxelData*)malloc(this->space_data.memory_size);

	printf("Voxel Count: %lld\n", this->space_data.voxel_count);
	printf("    Per-Voxel Memory: %lld Bytes\n", sizeof(VoxelData));
	printf("    Total Memory: %.2f MB\n\n", this->space_data.memory_size / 1000000.0f);

	this->LoadWorldVoxels();
	VoxelData data = this->space_data.space[this->space_data.voxel_count - 1];

	//exit(0);

	this->CheckCudaError((cudaError_enum)cudaMalloc(&this->space_data.space_cuda, this->space_data.memory_size), __FILE__, __LINE__);
	this->CheckCudaError((cudaError_enum)cudaMemcpy(this->space_data.space_cuda, this->space_data.space, this->space_data.memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
}

void WorldSpace::LoadWorldVoxels() {
	std::vector<int> lower{ 0, 0, 0 };
	std::vector<int> upper{ (int)this->space_data.world_size.x, (int)this->space_data.world_size.y, 10 };

	VoxelData ground_data{ 0, 0 };

	this->SetWorldRegion(lower, upper, ground_data);

	lower = std::vector<int>{ 480, 480, 60 };
	upper = std::vector<int>{ 520, 520, 110 };

	//ground_data = VoxelData{ 1, Transform::Vector4{ 255, 255, 255, 255 } };
	ground_data = VoxelData{ 1, 1 };

	this->SetWorldRegion(lower, upper, ground_data);
}

void WorldSpace::SetWorldRegion(std::vector<int> lower, std::vector<int> upper, VoxelData voxel_data) {

	for (int i = lower[0]; i < upper[0]; i++) {
		for (int j = lower[1]; j < upper[1]; j++) {
			for (int k = lower[2]; k < upper[2]; k++) {
				int index = indexer::index012(
					0, i, (int)this->space_data.world_size.x,
					0, j, (int)this->space_data.world_size.y,
					0, k, (int)this->space_data.world_size.z
				);

				this->space_data.space[index] = voxel_data;
			}
		}
	}
}

void WorldSpace::Cleanup() {
	printf("    Freeing cuda memory...\n");
	cudaFree(this->space_data.space_cuda);
	printf("    Freeing cpu memory...\n");
	free(this->space_data.space);
}

void WorldSpace::CheckCudaError(cudaError_enum result, const char* file, int line) {
	if (result) {
		fprintf(
			stderr,
			"CUDA error in %s at line %d.\n    [Error:%d %s] %s\n",
			file,
			line,
			static_cast<unsigned int>(result),
			(const char*)cudaGetErrorName((cudaError_t)result),
			cudaGetErrorString((cudaError_t)result)
		);
		exit(EXIT_FAILURE);
	}
}