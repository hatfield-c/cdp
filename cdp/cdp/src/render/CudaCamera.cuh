#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../engine/Transform.h"
#include "../engine/Quaternion.h"
#include "../engine/Indexer.h"
#include "../engine/VoxelData.h"
#include "../engine/RaycastHitData.h"
#include "../engine/SpaceData.h"
#include "../engine/WorldSpace.h"
#include "../entity/CameraData.h"

__device__ Vector3 GetCameraRayDirection(CameraData camera_data, Vector2 pixel_position);
__device__ RaycastHitData Raycast(SpaceData space_data, CameraData camera_data, Vector3 ray_direction, Vector2 pixel_position);
__global__ void RenderCamera_Kernel(CameraData camera_data, SpaceData space_data);
void RenderCamera(CameraData camera_data, WorldSpace* world_space);