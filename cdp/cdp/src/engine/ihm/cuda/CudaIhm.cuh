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
	__global__ void GetSimilarityScore_Kernel(IhmCortex ihm_cortex, int iteration, byte difference_threshold, byte* difference_vector, double* score_buffer);
	__global__ void SmallestIndexReduction_Kernel(IhmCortex ihm_cortex, int iteration, byte* difference_vector, unsigned long long* index_buffer);
	__global__ void GetDifferenceVector_Kernel(IhmCortex ihm_cortex, byte* phash, byte* difference_vector, unsigned long long difference_threshold = 0);
	__global__ void GenerateIhm_Kernel(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm);
	__global__ void RenderHeatmap_Kernel(SpaceData space_data, IhmRenderer ihm_renderer, IhmGenerator ihm_generator, IhmGenerator slice_generator, byte* ihm, byte* ihm_slice, double* score_buffer, byte* img, int direction_index, int height);
	__global__ void FilterLocalOffsets_Kernel(IhmCortex ihm_cortex, IhmGenerator ihm_generator, byte* difference_vector, unsigned long long* index_matrix0, unsigned long long* index_matrix1, unsigned long long* index_matrix2, int direction_index, Vector3 anchor);
	double GetSimilarityScore(IhmCortex ihm_cortex, byte* phash, bool is_verbose = false);
	unsigned long long FindIhmIndex(IhmCortex ihm_cortex, byte* phash, bool is_verbose = false);
	unsigned long long SmallestIndexReduction(IhmCortex ihm_cortex, byte* difference_vector, bool is_verbose = false);
	byte* GetDifferenceVector(IhmCortex ihm_cortex, byte* phash, bool is_verbose = false, unsigned long long difference_threshold = 0);
	void GenerateIhm(SpaceData space_data, Camera camera, IhmGenerator ihm_generator, byte* ihm);
	void RenderHeatmap(SpaceData space_data, IhmRenderer ihm_renderer, IhmGenerator ihm_generator, IhmGenerator slice_generator, byte* ihm, byte* ihm_slice, byte* img, int direction_index, int height);
	Vector3* EstimatePosition(IhmCortex ihm_cortex, IhmGenerator ihm_generator, byte* sensor_phash, Vector3 anchor, int direction_index);
}
