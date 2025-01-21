#include "CpuEngine.h"

CpuEngine::CpuEngine(std::vector<CUdeviceptr> depth_textures, std::vector<CUdeviceptr> phash_textures, std::vector<CUdeviceptr> shaded_textures) {
	this->world_space = new WorldSpace();
	this->ihm_generator.Init(
		3,
		Vector::ZERO3(),
		this->world_space->space_data.world_size0,
		this->world_space->space_data.world_size0,
		Vector3{ 10, 10, 10 },
		Vector2{ 16, 16 }
	);

	for (int i = 0; i < depth_textures.size(); i++) {
		CUdeviceptr depth_texture = depth_textures[i];
		CUdeviceptr phash_texture = phash_textures[i];
		CUdeviceptr shaded_texture = shaded_textures[i];

		Camera* camera = new Camera{};
		camera->Init("camera " + i, depth_texture, phash_texture, shaded_texture);
		
		this->camera_list.push_back(camera);
	}

	this->drone_alpha.Init();
	this->drone_alpha.rigidbody.position = Vector3{ 48, 4, 42 };
	this->drone_alpha.rigidbody.rotation = Quaternion::QuaternionFromDirection(this->ihm_generator.directions_cpu[12]);
	this->drone_alpha.rigidbody.velocity.z = 1;
	this->drone_alpha.rigidbody.angular_velocity.y = -0.1;
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

	std::chrono::steady_clock::duration frame_time_passed = std::chrono::steady_clock::now() - this->frame_begin_time;
	unsigned long long time_lapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time_passed).count();
	unsigned long long delta_time_steps = Physics::DeltaTimeMilli();

	if (time_lapsed < delta_time_steps) {
		unsigned long long sleep_time = delta_time_steps - time_lapsed;
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
	}

	this->frame_begin_time = std::chrono::steady_clock::now();
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

	if (gui_data.control_index != 0) {
		IhmState ihm_state = this->ihm_generator.GetIhmState(gui_data.ihm_index, false);

		this->camera_list[0]->transform.position = ihm_state.position;
		this->camera_list[0]->transform.rotation = ihm_state.rotation;
	}
	else {
		this->camera_list[0]->transform.position = this->drone_alpha.rigidbody.position * this->ihm_generator.world_stride;
		this->camera_list[0]->transform.rotation = this->drone_alpha.rigidbody.rotation;

		gui_data.camera_position = this->drone_alpha.rigidbody.position;
	}

	/*printf(
		"[%lld] Pos:(%.2f, %.2f, %.2f) Rot:(%.2f, %.2f, %.2f, %.2f) Vel:(%.2f, %.2f, %.2f) AnV:(%.2f, %.2f, %.2f)\n",
		gui_data.ihm_index,
		this->camera_list[0]->transform.position.x, this->camera_list[0]->transform.position.y, this->camera_list[0]->transform.position.z,
		this->camera_list[0]->transform.rotation.x, this->camera_list[0]->transform.rotation.y, this->camera_list[0]->transform.rotation.z, this->camera_list[0]->transform.rotation.w,
		this->drone_alpha.rigidbody.velocity.x, this->drone_alpha.rigidbody.velocity.y, this->drone_alpha.rigidbody.velocity.z,
		this->drone_alpha.rigidbody.angular_velocity.x, this->drone_alpha.rigidbody.angular_velocity.y, this->drone_alpha.rigidbody.angular_velocity.z
	);*/
}

void CpuEngine::PhysicsUpdate(GuiData gui_data) {
	Vector3 wind = Vector::ZERO3();

	//this->drone_alpha.rigidbody.Accelerate(Physics::Gravity());
	this->drone_alpha.rigidbody.AirResistance(wind);
	this->drone_alpha.rigidbody.Update();
}

void CpuEngine::RenderUpdate(GuiData gui_data) {
	int camera_index = gui_data.camera_index;

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

	CudaIhm::GenerateIhm(this->world_space->space_data, camera, this->ihm_generator, ihm);

	printf("    Copying IHM to CPU...\n");
	CudaError::CheckError((cudaError_enum)cudaMemcpy(ihm_cpu, ihm, memory_size, cudaMemcpyDeviceToHost), __FILE__, __LINE__);

	printf("    Saving IHM to disk...\n");
	FILE* out_file;
	fopen_s(&out_file, gui_data.save_ihm_path.c_str(), "wb");
	if (out_file == NULL) {
		printf("\n\nWarning: File did not open when saving IHM:\n    %s!\n", gui_data.save_ihm_path.c_str());
		exit(1);
	}
	int result = fwrite(ihm_cpu, sizeof(byte), memory_size, out_file);
	fclose(out_file);

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

	FILE* in_file;
	fopen_s(&in_file, gui_data.load_ihm_path.c_str(), "rb");
	if (in_file == NULL) {
		printf("\n\nWarning: File did not open when loading IHM:\n    %s!\n", gui_data.load_ihm_path.c_str());
		exit(1);
	}
	int result = fread(ihm_cpu, sizeof(byte), memory_size, in_file);
	fclose(in_file);

	printf("    Copying IHM to GPU...\n");
	CudaError::CheckError((cudaError_enum)cudaMemcpy(ihm, ihm_cpu, memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);

	this->ihm_cortex.Init(ihm, ihm_cpu, this->ihm_generator.state_count);

	printf("    Done!\n\n");
}

void CpuEngine::VerifyIhm(GuiData gui_data) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	unsigned long long smallest_index = CudaIhm::FindIhmIndex(this->ihm_cortex, this->camera_list[0]->phash_data, true);
	printf("\nSmallest Index: %lld\n\n", smallest_index);

	double score = CudaIhm::GetSimilarityScore(this->ihm_cortex, this->camera_list[0]->phash_data, true);
	printf("\nSimilarity Score: %f\n", score);

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	printf("        Time Elapsed: %d s\n", time_lapsed);
}

void CpuEngine::SaveSimilarityHeatMap(GuiData gui_data) {
	Vector3 render_stride{ 1, 1, 1 };
	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };
	std::string base_path = "./data/results/heat_";

	Camera camera = *this->camera_list[0];
	unsigned long long byte_count = render_size.x * render_size.y * 3;
	byte* img_cpu = new byte[byte_count];
	byte* img;

	CudaError::CheckError((cudaError_enum)cudaMalloc(&img, byte_count), __FILE__, __LINE__);

	for (int k = 40; k < this->world_space->space_data.world_size0.y - 10; k += 10) {
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		std::string save_path = base_path + std::to_string(k) + ".jpg";

		printf("\nCreating image: %s\n", save_path.c_str());
		IhmGenerator heatmap_generator{};
		heatmap_generator.Init(
			3,
			Vector3{ 0, (float)k, 0 },
			Vector3{ render_size.x, 1, render_size.y},
			this->world_space->space_data.world_size0,
			render_stride,
			Vector2{ 16, 16 }
		);

		IhmRenderer ihm_renderer{};
		ihm_renderer.Init(
			Vector2{ heatmap_generator.world_width.x, heatmap_generator.world_width.z },
			Vector2{ render_stride.x, render_stride.z }
		);

		unsigned long long memory_size = heatmap_generator.bit_count * sizeof(byte);
		byte* heatmap_ihm;
		CudaError::CheckError((cudaError_enum)cudaMalloc(&heatmap_ihm, memory_size), __FILE__, __LINE__);

		CudaIhm::GenerateIhm(this->world_space->space_data, camera, heatmap_generator, heatmap_ihm);
		CudaIhm::RenderHeatmap(this->world_space->space_data, ihm_renderer, this->ihm_generator, heatmap_generator, this->ihm_cortex.ihm, heatmap_ihm, img, 12, k);

		CudaError::CheckError((cudaError_enum)cudaMemcpy(img_cpu, img, byte_count, cudaMemcpyDeviceToHost), __FILE__, __LINE__);

		for (int i = 0; i < render_size.y; i++) {
			for (int j = 0; j < render_size.x; j++) {
				unsigned long long index = Indexer::FlatIndex3(j, k, i, this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.y);

				Vector3 voxel_position{ j, k, i };
				VoxelData voxel_data = CudaWorld::ReadVoxel(this->world_space->space_data, voxel_position);

				if (voxel_data.entity_id > 0) {
					unsigned long long r_index = Indexer::FlatIndex3(0, j, i, 3, render_size.x);
					unsigned long long g_index = Indexer::FlatIndex3(1, j, i, 3, render_size.x);
					unsigned long long b_index = Indexer::FlatIndex3(2, j, i, 3, render_size.x);
					img_cpu[r_index] = 255;
					img_cpu[g_index] = 255;
					img_cpu[b_index] = 255;
				}
			}
		}

		int result = stbi_write_jpg(save_path.c_str(), render_size.x, render_size.y, 3, img_cpu, 100);
		cudaFree(heatmap_ihm);

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
		printf("    Done!\n", time_lapsed);
		printf("        Time Elapsed: %d s\n", time_lapsed);
	}

	cudaFree(img);

	printf("    Done!\n");
}

void CpuEngine::SaveConfusionMap(GuiData gui_data) {
	

	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };
	Vector3 render_size_3d{ this->world_space->space_data.world_size0.x, 1, this->world_space->space_data.world_size0.z };
	Vector3 chunk_size{ 100, 1, 100 };
	Vector3 chunk_count = (render_size_3d / chunk_size).Ceil();
	Vector3 chunked_size = chunk_count * chunk_size;
	std::string save_path = "./data/results/confusion_map.jpg";

	Camera camera = *this->camera_list[0];
	unsigned long long byte_count = chunked_size.x * chunked_size.z * 3;
	byte* img_cpu = new byte[byte_count];
	memset(img_cpu, 0, byte_count);

	int k = 40;

	for (int w = 0; w < chunk_count.x; w++) {
		for (int h = 0; h < chunk_count.z; h++) {

			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

			for (int i = 0; i < chunk_size.x; i += 1) {
				printf("*");
				if (i >= render_size.y) {
					continue;
				}

				for (int j = 0; j < chunk_size.z; j += 1) {
					if (j >= render_size.x) {
						continue;
					}

					unsigned long long x_index = Indexer::FlatIndex2(j, w, chunk_size.x);
					unsigned long long y_index = Indexer::FlatIndex2(i, h, chunk_size.z);

					unsigned long long r_index = Indexer::FlatIndex3(0, x_index, y_index, 3, render_size.x);
					unsigned long long g_index = Indexer::FlatIndex3(1, x_index, y_index, 3, render_size.x);
					unsigned long long b_index = Indexer::FlatIndex3(2, x_index, y_index, 3, render_size.x);

					Vector3 voxel_position{ x_index, k, y_index };
					VoxelData voxel_data = CudaWorld::ReadVoxel(this->world_space->space_data, voxel_position);

					if (voxel_data.entity_id > 0) {
						img_cpu[r_index] = 255;
						img_cpu[g_index] = 255;
						img_cpu[b_index] = 255;
					}
					else {
						int direction_index = 12;// this->ihm_generator.GetClosestDirectionIndex(Quaternion::RotatePoint(Vector::RIGHT(), camera.transform.rotation));

						this->camera_list[0]->transform.position = Vector3{ (float)x_index, (float)k, (float)y_index };
						this->camera_list[0]->transform.rotation = Quaternion::QuaternionFromDirection(this->ihm_generator.directions_cpu[direction_index]);
						this->RenderUpdate(gui_data);

						Vector3 anchor = this->camera_list[0]->transform.position / this->ihm_generator.world_stride;
						anchor = anchor.Floor();
						anchor.x += 0;
						anchor.z += 0;

						Vector3* estimates = CudaIhm::EstimatePosition(this->ihm_cortex, this->ihm_generator, this->camera_list[0]->phash_data, anchor, direction_index, false);
						Vector3 estimate = estimates[0];

						if (Transform::Norm3(estimate) == 0) {
							estimate = estimates[1];

							if (Transform::Norm3(estimate) == 0) {
								estimate = estimates[2];
							}
						}

						float score = Transform::Norm3(this->camera_list[0]->transform.position - estimate);
						score = (255.0 / 100.0) * score;
						score = Math::Clip(score, 0.0, 255.0);

						int r_val = (int)score;
						int g_val = 255 - r_val;

						img_cpu[r_index] = r_val;
						img_cpu[g_index] = g_val;
					}
				}
			}
			int result = stbi_write_jpg(save_path.c_str(), chunked_size.x, chunked_size.z, 3, img_cpu, 100);

			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
			printf("        Saving chunk...\n");
			printf("            Chunk Time: %d s\n", time_lapsed);
		}
	}

	free(img_cpu);

	printf("    Done!\n");
}

void CpuEngine::EstimatePositionIhm(GuiData gui_data) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	IhmState ihm_state = this->ihm_generator.GetIhmState(gui_data.ihm_index, false);
	Vector3 anchor = this->camera_list[0]->transform.position / this->ihm_generator.world_stride;
	anchor.x += 2;
	anchor.z += 3;
	Vector3* estimates = CudaIhm::EstimatePosition(this->ihm_cortex, this->ihm_generator, this->camera_list[0]->phash_data, anchor, ihm_state.direction_index);
	
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	printf("        Time Elapsed: %d s\n", time_lapsed);

	printf("\nEstimated Position0: {%.2f, %.2f, %.2f}\n", estimates[0].x, estimates[0].y, estimates[0].z);
	printf("Estimated Position1: {%.2f, %.2f, %.2f}\n", estimates[1].x, estimates[1].y, estimates[1].z);
	printf("Estimated Position1: {%.2f, %.2f, %.2f}\n\n", estimates[2].x, estimates[2].y, estimates[2].z);

	//free(estimates);
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