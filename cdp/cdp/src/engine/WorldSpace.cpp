#include "WorldSpace.h"

WorldSpace::WorldSpace() {
	this->space_data.voxel_count = this->space_data.world_size.x * this->space_data.world_size.y * this->space_data.world_size.z;
	this->space = new VoxelData[this->space_data.voxel_count];

	this->LoadWorldVoxels();

	int size = this->space_data.voxel_count * sizeof(VoxelData);
	cudaMalloc(&this->space_cuda, size);
	cudaMemcpy(this->space_cuda, this->space, size, cudaMemcpyHostToDevice);
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

				this->space[index] = voxel_data;
			}
		}
	}
}

void WorldSpace::Cleanup() {
	cudaFree(this->space_cuda);
	free(this->space);
}