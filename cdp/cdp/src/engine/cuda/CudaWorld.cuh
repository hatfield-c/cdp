#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"
#include "../VoxelData.h"
#include "../SpaceData.h"
#include "../../system/CudaError.h"

__global__ void AssignChunk_Kernel(SpaceData space_data, VoxelData voxel_data, Vector3 lower, Vector3 upper);
__global__ void AssignPoints_Kernel(SpaceData space_data, Vector3* points, int point_count, VoxelData voxel_data);
void AssignChunk(SpaceData space_data, VoxelData voxel_data, Vector3 lower, Vector3 upper);
void AssignAll(SpaceData space_data, VoxelData voxel_data);
void AssignPoints(SpaceData space_data, Vector3* points, int point_count, VoxelData voxel_data);