#include "WorldSpace.h"

WorldSpace::WorldSpace() {
	this->space_data.voxel_count = (unsigned long long)(this->space_data.world_size.x * this->space_data.world_size.y * this->space_data.world_size.z);
	this->space_data.memory_size = this->space_data.voxel_count * sizeof(VoxelData);

	this->space_data.space = (VoxelData*)malloc(this->space_data.memory_size);

	printf("Voxel Count: %lld\n", this->space_data.voxel_count);
	printf("    Per-Voxel Memory: %lld Bytes\n", sizeof(VoxelData));
	printf("    Total Memory: %.2f MB\n\n", this->space_data.memory_size / 1000000.0f);

	printf("Initializing Voxel World...\n");

	CudaError::CheckError((cudaError_enum)cudaMalloc(&this->space_data.space_cuda, this->space_data.memory_size), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaMemcpy(this->space_data.space_cuda, this->space_data.space, this->space_data.memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);

	this->InitWorldVoxels(true);

	printf("    Done!\n");
}

void WorldSpace::InitWorldVoxels(bool is_debug_cube) {
	Vector3 lower{ 0, 0, 0 };
	Vector3 upper{ this->space_data.world_size.x, this->space_data.world_size.y, this->space_data.world_size.z };
	VoxelData init_data{ 0, Vector4{ 0, 0, 0, 0 } };

	AssignChunk(this->space_data, init_data, lower, upper);

	if (is_debug_cube) {
		lower = Vector3{ 480, 60, 480 };
		upper = Vector3{ 520, 110, 520 };
		VoxelData ground_data{ 1, Vector4{ 255, 255, 255, 255 } };

		AssignChunk(this->space_data, ground_data, lower, upper);
	}
	
}

void WorldSpace::SetWorldRegion(Vector3 lower, Vector3 upper, VoxelData voxel_data) {

	for (int i = lower.x; i < upper.x; i++) {
		for (int j = lower.y; j < upper.y; j++) {
			for (int k = lower.z; k < upper.z; k++) {
				int index = Indexer::FlatIndex3(i, j, k, (int)this->space_data.world_size.x, (int)this->space_data.world_size.y, (int)this->space_data.world_size.z);

				this->space_data.space[index] = voxel_data;
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