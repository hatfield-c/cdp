#pragma once

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../../SpaceData.h"
#include "../../../entity/Camera.h"
#include "../IhmGenerator.h"
#include "../IhmRenderer.h"
#include "../IhmCortex.h"

namespace CudaIhm {
	__device__ void SyncThreads();
	__global__ void SmallestIndexReduction_Kernel(IhmCortex ihm_cortex, int iteration, byte* difference_vector, unsigned long long* index_buffer);
	__global__ void GetDifferenceVector_Kernel(IhmCortex ihm_cortex, byte* phash, byte* difference_vector, unsigned long long difference_threshold = 0);
	__global__ void GetChamferDistances_Kernel(IhmCortex ihm_cortex, IhmGenerator ihm_generator, Vector3* cloud, float* distances, float* buffer, Vector3 lower, Vector3 upper, unsigned long long thread_units);
	__global__ void GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm);
	__global__ void ExtractRenderClouds_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm, Vector3* ihm_clouds);
	unsigned long long FindIhmIndex(IhmCortex ihm_cortex, byte* phash, bool is_verbose = false);
	unsigned long long SmallestIndexReduction(IhmCortex ihm_cortex, byte* difference_vector, bool is_verbose = false);
	byte* GetDifferenceVector(IhmCortex ihm_cortex, byte* phash, bool is_verbose = false, unsigned long long difference_threshold = 0);
	float* GetChamferDistances(IhmCortex ihm_cortex, IhmGenerator ihm_generator, Vector3* cloud, Vector3 lower, Vector3 upper);
	void ExtractRenderClouds(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm, Vector3* ihm_clouds);
	void GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm);
	Vector3* EstimatePosition(IhmCortex ihm_cortex, IhmGenerator ihm_generator, byte* sensor_phash, Vector3 anchor, int direction_index, bool is_verbose = true);
}
