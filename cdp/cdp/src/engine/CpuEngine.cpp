#include "CpuEngine.h"

CpuEngine::CpuEngine(std::vector<CUdeviceptr> depth_textures, std::vector<CUdeviceptr> phash_textures, std::vector<CUdeviceptr> shaded_textures) {
	this->world_space = new WorldSpace();
	this->ihm_generator.Init();

	for (int i = 0; i < depth_textures.size(); i++) {
		CUdeviceptr depth_texture = depth_textures[i];
		CUdeviceptr phash_texture = phash_textures[i];
		CUdeviceptr shaded_texture = shaded_textures[i];

		Camera* camera = new Camera{};// new Camera("camera " + i, gpu_texture);
		camera->Init("camera " + i, depth_texture, phash_texture, shaded_texture);
		
		this->camera_list.push_back(camera);
	}
}

void CpuEngine::Start(GuiData gui_data) {
	this->cycle_count = 0;

	this->camera_list[0]->transform.position = Vector3{ 780, 40, 300 };
	this->camera_list[0]->transform.rotation = Quaternion::QuaternionFromEulerAngles(Vector3{ 0, 1.35, 0 });
}

void CpuEngine::Update(GuiData gui_data) {
	this->ScenarioUpdate(gui_data);
	this->PhysicsUpdate(gui_data);
	this->RenderUpdate(gui_data);
}

void CpuEngine::End(GuiData gui_data) {
	
}

void CpuEngine::ScenarioUpdate(GuiData gui_data) {
	/*Vector3 target_location{480, 40, 420};

	Vector3 offset = this->camera_list[0]->transform.position - target_location;
	Vector4 rotation_amount = Quaternion::QuaternionFromEulerAngles(Vector3{ 0, 0.003, 0 });
	Vector3 new_position = Quaternion::RotatePoint(offset, rotation_amount) + target_location;

	this->camera_list[0]->transform.position = new_position;

	Vector4 camera_rotation = Quaternion::MultiplyQuaternions(rotation_amount, this->camera_list[0]->transform.rotation, false);

	this->camera_list[0]->transform.rotation = camera_rotation;
	this->camera_list[0]->vote_threshold = gui_data.vote_threshold;
	*/

	IhmState ihm_state = this->ihm_generator.GetIhmState(gui_data.ihm_position_index, false);

	this->camera_list[0]->transform.position = ihm_state.position;
	this->camera_list[0]->transform.rotation = ihm_state.rotation;
	this->camera_list[0]->vote_threshold = gui_data.vote_threshold;

	//printf("(%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f, %.2f)\n", 
		//this->camera_list[0]->transform.position.x, this->camera_list[0]->transform.position.y, this->camera_list[0]->transform.position.z,
		//this->camera_list[0]->transform.rotation.x, this->camera_list[0]->transform.rotation.y, this->camera_list[0]->transform.rotation.z, this->camera_list[0]->transform.rotation.w
	//);
}

void CpuEngine::PhysicsUpdate(GuiData gui_data) {

}

void CpuEngine::RenderUpdate(GuiData gui_data) {
	int camera_index = gui_data.camera_index;

	CudaCamera::DepthUpdate(*this->camera_list[camera_index], this->world_space->space_data);
	cudaDeviceSynchronize();
	CudaCamera::RenderCamera(*this->camera_list[camera_index], this->world_space->space_data);
	cudaDeviceSynchronize();
}

void CpuEngine::GenerateIhm(GuiData gui_data) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	
	printf("Saving IHM at path: %s\n", gui_data.save_ihm_path.c_str());

	Camera camera = *this->camera_list[0];
	unsigned long long memory_size = this->ihm_generator.bit_count * sizeof(byte);
	printf("    Allocating IHM CPU memory...\n");
	printf("        Size: %.2f MB\n", memory_size / 1000000.0);
	byte* ihm_cpu = new byte[memory_size];

	printf("    Allocating IHM GPU memory...\n");
	byte* ihm;
	CudaError::CheckError((cudaError_enum)cudaMalloc(&ihm, memory_size), __FILE__, __LINE__);

	printf("    Generating IHM...\n");
	CudaIhm::GenerateIhm(this->world_space->space_data, camera, this->ihm_generator, ihm);

	printf("    Copying IHM to CPU...\n");
	CudaError::CheckError((cudaError_enum)cudaMemcpy(ihm_cpu, ihm, memory_size, cudaMemcpyDeviceToHost), __FILE__, __LINE__);

	printf("    Saving IHM to disk...\n");
	printf("        Progress (Max 20 *): ");
	simple::file_ostream<std::true_type> out(gui_data.save_ihm_path.c_str());
	for (unsigned long long i = 0; i < memory_size; i++) {
		out << ihm_cpu[i];

		if (i % (int)(memory_size / 20) == 0) {
			printf("*");
		}
	}
	printf("\n");

	out.flush();
	out.close();

	printf("    Done!\n\n");

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	printf("        Time Elapsed: %d s\n", time_lapsed);
}

void CpuEngine::LoadIhm(GuiData gui_data) {
	printf("Loading IHM at path: %s\n", gui_data.load_ihm_path.c_str());

	unsigned long long memory_size = this->ihm_generator.bit_count * sizeof(byte);
	printf("    Allocating IHM CPU memory...\n");
	printf("        Size: %.2f MB\n", memory_size / 1000000.0);
	byte* ihm_cpu = new byte[memory_size];

	printf("    Allocating IHM GPU memory...\n");
	byte* ihm;
	CudaError::CheckError((cudaError_enum)cudaMalloc(&ihm, memory_size), __FILE__, __LINE__);

	printf("    Loading IHM from disk...\n");
	printf("        Progress (Max 20 *): ");
	simple::file_istream<std::true_type> in(gui_data.load_ihm_path.c_str());
	for (unsigned long long i = 0; i < memory_size; i++) {
		in >> ihm_cpu[i];

		if (i % (int)(memory_size / 20) == 0) {
			printf("*");
		}
	}
	printf("\n");

	printf("    Copying IHM to GPU...\n");
	CudaError::CheckError((cudaError_enum)cudaMemcpy(ihm, ihm_cpu, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);

	this->ihm_cortex.Init(ihm, ihm_cpu, this->ihm_generator.state_count);

	printf("    Done!\n\n");
}

void CpuEngine::VerifyIhm(GuiData gui_data) {
	unsigned long long smallest_index = CudaIhm::FindIhmIndex(this->ihm_cortex, this->camera_list[0]->phash_data, true);
	printf("\nSmallest Index: %lld\n", smallest_index);
	double score = CudaIhm::GetSimilarityScore(this->ihm_cortex, this->camera_list[0]->phash_data, true);
	printf("\nSimilarity Score: %f\n", score);
}

void CpuEngine::SaveSimilarityHeatMap(GuiData gui_data) {
	Vector3 img_size{ this->world_space->space_data.world_size.x, this->world_space->space_data.world_size.z, 3 };
	unsigned long long byte_count = img_size.x * img_size.y * img_size.z;
	byte* img = new byte[byte_count];

	std::string base_path = "./data/results/heat_";
	printf("Saving Heatmap at location: %sX.jpg\n", base_path.c_str());

	for (int k = 4; k < this->world_space->space_data.world_size.y - 10; k++) {
		
		std::string save_path = base_path + std::to_string(k) + ".jpg";
		
		memset(img, 0, byte_count);

		for (int i = 0; i < img_size.y; i++) {
			for (int j = 0; j < img_size.x; j++) {
				unsigned long long world_index = Indexer::FlatIndex3(
					j,
					k,
					i,
					this->ihm_generator.world_size.x,
					this->ihm_generator.world_size.y
				);

				VoxelData voxel_data = this->world_space->space_data.space[world_index];

				if (voxel_data.entity_id > 0) {
					unsigned long long r_index = Indexer::FlatIndex3(0, i, j, img_size.z, img_size.x);
					unsigned long long g_index = Indexer::FlatIndex3(1, i, j, img_size.z, img_size.x);
					unsigned long long b_index = Indexer::FlatIndex3(2, i, j, img_size.z, img_size.x);

					img[r_index] = 255;
					img[g_index] = 255;
					img[b_index] = 255;
				}
			}
		}

		int result = stbi_write_jpg(save_path.c_str(), img_size.x, img_size.y, img_size.z, img, 100);
	}

	printf("    Done!\n");
}

void CpuEngine::Cleanup() {
	printf("Cleaning CudaEngine...\n");
	printf("    Freeing GPU IHM...\n");
	cudaFree(this->ihm_cortex.ihm);
	printf("    Freeing CPU IHM...\n");
	free(this->ihm_cortex.ihm_cpu);

	this->world_space->Cleanup();
	printf("    Done!\n");
}