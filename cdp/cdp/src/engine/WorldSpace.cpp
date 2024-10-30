#include "WorldSpace.h"

#include <iostream>

WorldSpace::WorldSpace() {
	this->space_data.voxel_count = (unsigned long long)(this->space_data.world_size.x * this->space_data.world_size.y * this->space_data.world_size.z);
	this->space_data.memory_size = this->space_data.voxel_count * sizeof(VoxelData);

	this->space_data.space = (VoxelData*)malloc(this->space_data.memory_size);

	printf("Voxel Count: %lld\n", this->space_data.voxel_count);
	printf("    Per-Voxel Memory: %lld Bytes\n", sizeof(VoxelData));
	printf("    Total Memory: %.2f MB\n\n", this->space_data.memory_size / 1000000.0f);

	printf("Loading Voxel World...\n");
	this->LoadWorldVoxels();
	printf("    Done!\n");

	this->CheckCudaError((cudaError_enum)cudaMalloc(&this->space_data.space_cuda, this->space_data.memory_size), __FILE__, __LINE__);
	this->CheckCudaError((cudaError_enum)cudaMemcpy(this->space_data.space_cuda, this->space_data.space, this->space_data.memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
}

void WorldSpace::LoadWorldVoxels() {
	Vector3 lower{ 0, 0, 0 };
	Vector3 upper{ this->space_data.world_size.x, this->space_data.world_size.y, this->space_data.world_size.z };

	VoxelData init_data{ 0, Vector4{ 0, 0, 0, 0 } };

	this->SetWorldRegion(lower, upper, init_data);

	lower = Vector3{ 480, 480, 60 };
	upper = Vector3{ 520, 520, 110 };

	VoxelData ground_data{ 1, Vector4{ 255, 255, 255, 255 } };

	this->SetWorldRegion(lower, upper, ground_data);
}

void WorldSpace::SetWorldRegion(Vector3 lower, Vector3 upper, VoxelData voxel_data) {

	for (int i = lower.x; i < upper.x; i++) {
		for (int j = lower.y; j < upper.y; j++) {
			for (int k = lower.z; k < upper.z; k++) {
				int index = FlatIndex3(i, j, k, (int)this->space_data.world_size.x, (int)this->space_data.world_size.y, (int)this->space_data.world_size.z);

				this->space_data.space[index].entity_id = voxel_data.entity_id;
				this->space_data.space[index].color.x = voxel_data.color.x;
				this->space_data.space[index].color.y = voxel_data.color.y;
				this->space_data.space[index].color.z = voxel_data.color.z;
				this->space_data.space[index].color.w = voxel_data.color.w;
			}
		}
	}
}

void WorldSpace::Cleanup() {
	printf("    Freeing cuda memory...\n");
	cudaFree(this->space_data.space_cuda);
	printf("    Freeing cpu memory...\n");
	free(this->space_data.space);
}

void WorldSpace::CheckCudaError(cudaError_enum result, const char* file, int line) {
	if (result) {
		fprintf(
			stderr,
			"CUDA error in %s at line %d.\n    [Error:%d %s] %s\n",
			file,
			line,
			static_cast<unsigned int>(result),
			(const char*)cudaGetErrorName((cudaError_t)result),
			cudaGetErrorString((cudaError_t)result)
		);
		exit(EXIT_FAILURE);
	}
}