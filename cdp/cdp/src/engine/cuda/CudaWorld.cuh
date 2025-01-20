#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <chrono>

#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"
#include "../VoxelData.h"
#include "../SpaceData.h"
#include "../SpaceBuilder.h"
#include "../../system/CudaError.h"

namespace CudaWorld {
	__global__ void FillBox_Kernel(SpaceBuilder space_builder, SpaceData space_data, VoxelData voxel_data, Vector3 origin, Vector3 width);
	__global__ void WritePoints_Kernel(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count, VoxelData voxel_data);
	__global__ void PlanarDensify_Kernel(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count);
	__global__ void StochasticSubtraction_Kernel(SpaceBuilder space_builder, SpaceData space_data);
	VoxelData ReadVoxel(SpaceData space_data, Vector3 position);
	void FillBox(SpaceBuilder space_builder, SpaceData space_data, VoxelData voxel_data, Vector3 lower, Vector3 upper);
	void AssignAll(SpaceBuilder space_builder, SpaceData space_data, VoxelData voxel_data);
	void WritePoints(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count, VoxelData voxel_data);
	void PlanarDensify(SpaceBuilder space_builder, SpaceData space_data, Vector3* points, unsigned long long point_count);
	void StochasticSubtraction(SpaceBuilder space_builder, SpaceData space_data);
}