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

	this->InitWorldMemory(false, true);

	printf("    Done!\n\n");
}

void WorldSpace::InitWorldMemory(bool is_debug_cube, bool is_floor) {
	Vector3 lower{ 0, 0, 0 };
	Vector3 upper{ this->space_data.world_size.x, this->space_data.world_size.y, this->space_data.world_size.z };
	VoxelData init_data{ 0, 0 };

	AssignChunk(this->space_data, init_data, lower, upper);

	if (is_debug_cube) {
		lower = Vector3{ 480, 60, 480 };
		upper = Vector3{ 520, 110, 520 };
		VoxelData cube_data{ 1, 1 };

		AssignChunk(this->space_data, cube_data, lower, upper);
	}

	if (is_floor) {
		lower = Vector3{ 0, 0, 0};
		upper = Vector3{ this->space_data.world_size.x, 3, this->space_data.world_size.z };
		VoxelData floor_data{ 1, 1 };

		AssignChunk(this->space_data, floor_data, lower, upper);
	}
}

void WorldSpace::LoadWorld(std::string load_path) {
	printf("Loading world data...\n");
	printf("    path: %s\n", load_path.c_str());
	happly::PLYData plyIn(load_path);
	
	printf("    Extracting point cloud...\n");
	std::vector<std::array<double, 3>> vertices = plyIn.getVertexPositions();
	printf("        Total Points: %d\n", (int)vertices.size());

	printf("    Writing points to GPU world space...\n");
	this->WritePointsToCuda(vertices);

	printf("    Done!\n\n");
}

void WorldSpace::WritePointsToCuda(std::vector<std::array<double, 3>> point_list) {
	int memory_size = point_list.size() * sizeof(Vector3);

	VoxelData voxel_data{ 1, 1 };
	Vector3* points = (Vector3*)malloc(memory_size);
	Vector3* points_cuda = nullptr;

	for (int i = 0; i < point_list.size(); i++) {
		std::array<double, 3> point = point_list[i];

		points[i] = Vector3{ 
			(float)point[0] * this->space_data.indices_per_meter,
			(float)point[2] * this->space_data.indices_per_meter,
			(float)point[1] * this->space_data.indices_per_meter
		};
	}

	CudaError::CheckError((cudaError_enum)cudaMalloc(&points_cuda, memory_size), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaMemcpy(points_cuda, points, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);

	AssignPoints(this->space_data, points_cuda, (int)point_list.size(), voxel_data);
}

void WorldSpace::SetWorldRegion(Vector3 lower, Vector3 upper, VoxelData voxel_data) {

	for (int i = lower.x; i < upper.x; i++) {
		for (int j = lower.y; j < upper.y; j++) {
			for (int k = lower.z; k < upper.z; k++) {
				int index = Indexer::FlatIndex3(i, j, k, this->space_data.world_size.x, this->space_data.world_size.y);

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