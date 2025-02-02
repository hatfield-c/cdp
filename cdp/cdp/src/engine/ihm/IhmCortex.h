#pragma once

//#ifndef __CUDACC__  
//#define __CUDACC__
//#endif

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../Transform.h"
#include "../Quaternion.h"
#include "../Indexer.h"
#include "IhmEstimate.h"

typedef unsigned char byte;

struct IhmCortex {
	int thread_units = 256;
	byte* ihm;
	byte* ihm_cpu;
	Vector3* ihm_clouds;
	
	float* pixel_distances;
	float* chamfer_distances;

	Vector3 search_radius{ 5, 5, 5 };
	Vector3 search_width{ 11, 11, 11 };
	Vector2 phash_size{ 16, 16 };
	int direction_count;
	unsigned long long phash_count;
	unsigned long long state_count;
	unsigned long long pixel_count;

	unsigned long long chamfer_memory_size;
	unsigned long long distance_memory_size;

	curandState_t curand_state;
	long seed;

	void Init(byte* ihm, byte* ihm_cpu, Vector3* ihm_clouds, int direction_count) {
		this->ihm = ihm;
		this->ihm_cpu = ihm_cpu;
		this->ihm_clouds = ihm_clouds;
		this->direction_count = direction_count;
		this->state_count = this->direction_count * this->search_width.Mult();
		this->phash_count = this->phash_size.x * this->phash_size.y;
		this->pixel_count = (this->phash_count / 4) * this->state_count;

		this->pixel_distances;
		this->distance_memory_size = pixel_count * sizeof(float);
		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->pixel_distances, this->distance_memory_size), __FILE__, __LINE__);

		this->chamfer_distances;
		this->chamfer_memory_size = state_count * sizeof(float);
		CudaError::CheckError((cudaError_enum)cudaMalloc(&this->chamfer_distances, this->chamfer_memory_size), __FILE__, __LINE__);

		this->ResetDistanceBuffers();

		this->seed = 5525;
	}

	IhmEstimate EstimateIhmState() {
		float* chamfer_distances = new float[this->state_count];
		int memory_size = this->state_count * sizeof(float);

		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(chamfer_distances, this->chamfer_distances, memory_size, cudaMemcpyDeviceToHost), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

		IhmEstimate estimate{};

		for (unsigned long long i = 0; i < this->state_count; i++) {
			Vector4 index_data = Indexer::InverseFlatIndex4(i, this->direction_count, this->search_width.x, this->search_width.y);

			float score = chamfer_distances[i];

			if (i == 14652) {
				//printf("[%d] %.2f ", i, score);
				//index_data.Print();
			}

			if (score < estimate.chamfer_score) {
				estimate.local_index = i;
				estimate.chamfer_score = score;
				estimate.direction_index = index_data.x;
				estimate.position = Vector3{ index_data.y, index_data.z, index_data.w };
			}
		}

		return estimate;
	}

	void ResetDistanceBuffers() {
		cudaMemset(this->pixel_distances, 0, this->distance_memory_size);
		cudaMemset(this->chamfer_distances, 0, this->chamfer_memory_size);
		CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
		CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
	}

	__host__ __device__ long NextSample(long current) {
		long next = current * 1103515245 + 12345;
		next = (unsigned)(next / 65536) % 32768;

		return next;
	}

	__device__ float SampleFloat(long sample) {
		return (float)sample / 32768.0;
	}

	__device__ void UpdateNearestDistances(IhmGenerator ihm_generator, Vector3* camera_cloud, Vector3 anchor) {
		unsigned long long region_pixel_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);
		this->pixel_distances[region_pixel_index] = 1000000000;
		
		Vector3 index_data = Indexer::InverseFlatIndex3(region_pixel_index, this->phash_size.x / 2, this->phash_size.y / 2);
		Vector2 offset{};

		long sample = this->NextSample(this->seed + region_pixel_index);
		float dice_roll = this->SampleFloat(sample);
		offset.x = round(dice_roll);

		sample = this->NextSample(sample);
		dice_roll = this->SampleFloat(sample);
		offset.y = round(dice_roll);

		Vector2 pixel_position = (Vector2{ index_data.x, index_data.y } * 2) + offset;
		
		unsigned long long region_state_index = index_data.z;
		
		Vector4 region_state_data = Indexer::InverseFlatIndex4(region_state_index, this->direction_count, this->search_width.x, this->search_width.y);
		int direction_index = region_state_data.x;
		Vector3 local_position{ region_state_data.y, region_state_data.z, region_state_data.w };
		Vector3 voxel_position = anchor - this->search_radius + local_position;

		unsigned long long ihm_state_index = Indexer::FlatIndex4(direction_index, voxel_position.x, voxel_position.y, voxel_position.z, ihm_generator.direction_count, ihm_generator.world_width_strided.x, ihm_generator.world_width_strided.y);

		if (!voxel_position.IsBounded(Vector::ZERO3(), ihm_generator.world_width_strided - 1)) {
			return;
		}

		unsigned long long index_a0 = Indexer::FlatIndex2(pixel_position.x, pixel_position.y, this->phash_size.x);
		unsigned long long index_b0 = Indexer::FlatIndex3(pixel_position.x, pixel_position.y, ihm_state_index, this->phash_size.x, this->phash_size.y);

		Vector3 a0 = camera_cloud[index_a0];
		Vector3 b0 = this->ihm_clouds[index_b0];

		float shortest_a = 1000000000000;
		float shortest_b = 1000000000000;
		for (int j = 0; j < ihm_generator.phash_count; j++) {
			unsigned long long index_a1 = j + Indexer::FlatIndex3(0, 0, ihm_state_index, this->phash_size.x, this->phash_size.y);
			unsigned long long index_b1 = j;

			Vector3 a1 = this->ihm_clouds[index_a1];
			Vector3 b1 = camera_cloud[index_b1];

			float a_distance = Transform::Norm3(a1 - a0);
			float b_distance = Transform::Norm3(b1 - b0);

			if (a_distance < shortest_a) {
				shortest_a = a_distance;
			}

			if (b_distance < shortest_b) {
				shortest_b = b_distance;
			}
		}

		this->pixel_distances[region_pixel_index] = shortest_a + shortest_b;

		//if (ihm_state_index == 1018764){//&& region_state_index == 15972) {
		//if (local_position == Vector3{ 5, 5, 5 }){
		//if (ihm_state_index == 1018764 && index_data.x == 0 && index_data.y == 0) {
			//printf("%.2f <%lld %lld> (%.2f %.2f)-(%.2f %.2f)-(%.2f %.2f) [%.2f %.2f %.2f] {%.2f %.2f %.2f} %ld\n", this->pixel_distances[ihm_state_index], region_state_index, ihm_state_index, pixel_position.x, pixel_position.y, offset.x, offset.y, index_data.x, index_data.y, local_position.x, local_position.y, local_position.z, voxel_position.x, voxel_position.y, voxel_position.z, this->seed);
		//}
	}

	__device__ void UpdateChamferDistances(IhmGenerator ihm_generator) {
		unsigned long long region_state_index = Indexer::FlatIndex2(threadIdx.x, blockIdx.x, blockDim.x);

		Vector4 index_data = Indexer::InverseFlatIndex4(region_state_index, this->direction_count, this->search_width.x, this->search_width.y);
		int direction_index = index_data.x;
		Vector3 local{ index_data.y, index_data.z, index_data.w };

		if (region_state_index >= this->state_count) {
			return;
		}

		float chamfer_distance = 0;
		for (int i = 0; i < this->phash_count / 4.0; i++) {
			unsigned long long pixel_index = i + Indexer::FlatIndex3(0, 0, region_state_index, this->phash_size.x / 2, this->phash_size.y / 2);

			chamfer_distance += this->pixel_distances[pixel_index];
		}

		this->chamfer_distances[region_state_index] = chamfer_distance / (2 * (this->phash_count / 4));

		if (local == Vector3{ 10, 10, 10 }){//&& index_data.x == 12) {
		//if(this->chamfer_distances[region_state_index] < 0.2){
			//printf("%.2f <%lld> [%.2f %.2f %.2f] %d\n", this->chamfer_distances[region_state_index], region_state_index, local.x, local.y, local.z, direction_index);
		}
	}

};