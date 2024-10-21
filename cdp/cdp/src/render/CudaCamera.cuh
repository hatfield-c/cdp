#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../engine/VoxelData.h"
#include "../engine/SpaceData.h"
#include "../engine/WorldSpace.h"
#include "../entity/CameraData.h"

__global__ void RenderCamera_Kernel(CameraData camera_data, VoxelData* voxel_data);

void RenderCamera(CameraData camera_data, WorldSpace* world_space);