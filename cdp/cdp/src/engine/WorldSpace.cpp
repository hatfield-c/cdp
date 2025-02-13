#include "WorldSpace.h"

WorldSpace::WorldSpace() {
	this->space_data.voxel_count0 = this->space_data.world_size0.Mult();
	this->space_data.voxel_count1 = this->space_data.world_size1.Mult();

	this->space_data.memory_size0 = this->space_data.voxel_count0 * sizeof(VoxelData);
	this->space_data.memory_size1 = this->space_data.voxel_count1 * sizeof(VoxelData);

	printf("World Data:\n");
	printf("    Per-Voxel Memory: %lld Bytes\n", sizeof(VoxelData));
	printf("    Voxel Count:\n");
	printf("        Level 0: %lld\n", this->space_data.voxel_count0);
	printf("        Level 1: %lld\n", this->space_data.voxel_count1);
	printf("    Total Memory:\n");
	printf("        Level 0: %.2f MB\n", this->space_data.memory_size0 / 1000000.0f);
	printf("        Level 1: %.2f MB\n", this->space_data.memory_size1 / 1000000.0f);

	printf("Initializing Voxel World...\n");

	CudaError::CheckError((cudaError_enum)cudaMalloc(&this->space_data.space0, this->space_data.memory_size0), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaMalloc(&this->space_data.space1, this->space_data.memory_size1), __FILE__, __LINE__);

	CudaError::CheckError((cudaError_enum)cudaMemset(this->space_data.space0, 0, this->space_data.memory_size0), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaMemset(this->space_data.space1, 0, this->space_data.memory_size1), __FILE__, __LINE__);

	this->InitWorldMemory(false, true);

	this->space_builder.Init();

	printf("    Done!\n\n");
}

void WorldSpace::InitWorldMemory(bool is_debug_cube, bool is_floor) {
	//VoxelData init_data{ 0, 0 };
	//CudaWorld::FillBox(this->space_builder, this->space_data, init_data, Vector::ZERO3(), this->space_data.world_size0);

	if (is_debug_cube) {
		Vector3 lower = Vector3{ 480, 60, 480 };
		Vector3 upper = Vector3{ 520, 110, 520 };
		Vector3 width = upper - lower;
		VoxelData cube_data{ 1, 1 };

		CudaWorld::FillBox(this->space_builder, this->space_data, cube_data, lower, width);
	}

	if (is_floor) {
		Vector3 lower = Vector3{ 0, 0, 0};
		Vector3 upper = Vector3{ this->space_data.world_size0.x, 3, this->space_data.world_size0.z };
		Vector3 width = upper - lower;
		VoxelData floor_data{ 1, 1 };

		CudaWorld::FillBox(this->space_builder, this->space_data, floor_data, lower, width);
	}
}

void WorldSpace::LoadWorld(std::string load_path) {
	printf("Loading world data...\n");
	printf("    path: %s\n", load_path.c_str());
	happly::PLYData plyIn(load_path);
	
	printf("    Extracting point cloud...\n");
	std::vector<std::array<double, 3>> vertices = plyIn.getVertexPositions();
	printf("        Total Points: %lld\n", (unsigned long long)vertices.size());

	printf("    Writing points to GPU world space...\n");
	Vector3* points_cuda = this->WritePointsToCuda(vertices);

	printf("    Build world state...\n");
	for (int i = 0; i < 1; i++) {
		if (i % 1 == 0) {
			printf("        Planar densify: Step %d...\n", i + 1);
		}

		CudaWorld::PlanarDensify(space_builder, this->space_data, points_cuda,(unsigned long long)vertices.size());
	}

	printf("    Done!\n\n");
}

Vector3* WorldSpace::WritePointsToCuda(std::vector<std::array<double, 3>> point_list) {
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

	CudaWorld::WritePoints(this->space_builder, this->space_data, points_cuda, (unsigned long long)point_list.size(), voxel_data);

	return points_cuda;
}

void WorldSpace::ActivateStochasticSubtraction() {
	CudaWorld::StochasticSubtraction(this->space_builder, this->space_data);

	Vector3 lower = Vector3{ 0, 0, 0 };
	Vector3 upper = Vector3{ this->space_data.world_size0.x, 3, this->space_data.world_size0.z };
	Vector3 width = upper - lower;
	VoxelData floor_data{ 1, 1 };

	CudaWorld::FillBox(this->space_builder, this->space_data, floor_data, lower, width);
}

void WorldSpace::Cleanup() {
	printf("    Freeing world space...\n");
	cudaFree(this->space_data.space0);
	cudaFree(this->space_data.space1);
}