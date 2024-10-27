#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../engine/VoxelData.h"
#include "../engine/RaycastHitData.h"
#include "../engine/SpaceData.h"
#include "../engine/WorldSpace.h"
#include "../entity/CameraData.h"

__device__ float Magnitude3(Transform::Vector3 vector);
__device__ float Magnitude4(Transform::Vector4 vector);
//__device__ Transform::Vector3 GetCameraRayDirection(CameraData camera_data, Transform::Vector2 pixel_position);
__device__ Transform::Vector3 GetCameraRayDirection(CameraData camera_data, Transform::Vector2 pixel_position);
__device__ Transform::Vector4 GetQuaternionConjugate(Transform::Vector4 original);
__device__ Transform::Vector4 MultiplyQuaternions(Transform::Vector4 q0, Transform::Vector4 q1, bool is_normalized);
__device__ Transform::Vector3 RotatePoint(Transform::Vector3 position, Transform::Vector4 quaternion);
__device__ RaycastHitData Raycast(SpaceData space_data, CameraData camera_data, Transform::Vector3 query_point, Transform::Vector3 ray_direction);
__device__ int GetIndexCWH(int c, int w, int h, int c_max, int w_max, int h_max);
__global__ void RenderCamera_Kernel(CameraData camera_data, SpaceData space_data);
void RenderCamera(CameraData camera_data, WorldSpace* world_space);