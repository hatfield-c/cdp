#pragma once

#include <vector>

#include "index.hpp"

#include "Voxel.h"

class WorldSpace {
	public:
		const float indices_per_meter = 10;
		const std::vector<int> world_size{ 1000, 1000, 300 };

		Voxel* space;

		WorldSpace();
		void LoadWorldVoxels();
		void SetWorldRegion(std::vector<int> v0, std::vector<int> v1, Voxel voxel_data);
};