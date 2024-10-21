#include "WorldSpace.h"

#include <iostream>

WorldSpace::WorldSpace() {
	this->space_data.voxel_count = (unsigned int)this->space_data.world_size.x * (unsigned int)this->space_data.world_size.y * (unsigned int)this->space_data.world_size.z;
	this->space_data.space = new VoxelData[this->space_data.voxel_count];

	this->LoadWorldVoxels();

	unsigned int size = ((unsigned int)this->space_data.voxel_count) * sizeof(VoxelData);
	//int size = 10 * sizeof(VoxelData);
	printf("%u\n", size);
	this->CheckCudaError((cudaError_enum)cudaMalloc(&this->space_data.space_cuda, size), __FILE__, __LINE__);
	this->CheckCudaError((cudaError_enum)cudaMemcpy(this->space_data.space_cuda, this->space_data.space, size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
}

void WorldSpace::LoadWorldVoxels() {
	std::vector<int> lower{ 0, 0, 0 };
	std::vector<int> upper{ (int)this->space_data.world_size.x, (int)this->space_data.world_size.y, 10 };

	VoxelData ground_data{ 0, 0 };

	this->SetWorldRegion(lower, upper, ground_data);

	lower = std::vector<int>{ 480, 480, 60 };
	upper = std::vector<int>{ 520, 520, 110 };

	ground_data = VoxelData{ 1, 0 };

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
	cudaFree(this->space_data.space_cuda);
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