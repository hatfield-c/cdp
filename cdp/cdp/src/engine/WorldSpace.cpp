#include "WorldSpace.h"

WorldSpace::WorldSpace() {
	this->LoadWorldVoxels();
}

void WorldSpace::LoadWorldVoxels() {
	std::vector<int> lower{ 0, 0, 0 };
	std::vector<int> upper{ this->world_size[0], this->world_size[1], 10 };

	Voxel ground_data{ 0, 0 };

	this->SetWorldRegion(lower, upper, ground_data);

	lower = std::vector<int>{ 480, 480, 60 };
	upper = std::vector<int>{ 520, 520, 110 };

	ground_data = Voxel{ 1, 0 };

	this->SetWorldRegion(lower, upper, ground_data);
}

void WorldSpace::SetWorldRegion(std::vector<int> lower, std::vector<int> upper, Voxel voxel_data) {
	for (int i = lower[0]; i < upper[0]; i++) {
		for (int j = lower[1]; j < upper[1]; j++) {
			for (int k = lower[2]; k < upper[2]; k++) {
				int index = indexer::index012(
					0, i, this->world_size[0],
					0, j, this->world_size[1],
					0, k, this->world_size[2]
				);

				//this->space[index] = voxel_data;
			}
		}
	}
}