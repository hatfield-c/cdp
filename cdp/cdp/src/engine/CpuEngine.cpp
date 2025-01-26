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
	this->drone_alpha.rigidbody.angular_velocity.y = -0.2;
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

	// debug code
	//std::cin.ignore();

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

void CpuEngine::SaveConfusionMap(GuiData gui_data) {
	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };
	Vector3 render_size_3d{ this->world_space->space_data.world_size0.x, 1, this->world_space->space_data.world_size0.z };
	Vector3 chunk_size{ 100, 1, 100 };
	Vector3 chunk_count = (render_size_3d / chunk_size).Ceil();
	Vector3 chunked_size = chunk_count * chunk_size;
	std::string save_path = "./data/results/confusion_map.jpg";

	//Vector2 chunks_lower{ 0, 0 };
	//Vector2 chunks_upper{ chunk_count.x, chunk_count.z };
	Vector2 chunks_lower{ 2, 3 };
	Vector2 chunks_upper{ 3, 4 };

	Camera camera = *this->camera_list[0];
	unsigned long long byte_count = chunked_size.x * chunked_size.z * 3;
	byte* img_cpu = new byte[byte_count];
	memset(img_cpu, 0, byte_count);

	VoxelData* world_voxels = new VoxelData[this->world_space->space_data.voxel_count0];
	byte_count = this->world_space->space_data.voxel_count0 * sizeof(VoxelData);
	memset(world_voxels, 0, byte_count);
	CudaError::CheckError((cudaError_enum)cudaMemcpy(world_voxels, this->world_space->space_data.space0, byte_count, cudaMemcpyDeviceToHost), __FILE__, __LINE__);

	int k = 40;

	for (int w = chunks_lower.x; w < chunks_upper.x; w++) {
		for (int h = chunks_lower.y; h < chunks_upper.y; h++) {
			printf("(%d, %d)\n", w, h);
			printf("    Progress: ");

			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

			for (int i = 0; i < chunk_size.x; i += 1) {
				printf("*");

				for (int j = 0; j < chunk_size.z; j += 1) {

					unsigned long long x_index = Indexer::FlatIndex2(j, w, chunk_size.x);
					unsigned long long y_index = Indexer::FlatIndex2(i, h, chunk_size.z);

					unsigned long long r_index = Indexer::FlatIndex3(0, x_index, y_index, 3, render_size.x);
					unsigned long long g_index = Indexer::FlatIndex3(1, x_index, y_index, 3, render_size.x);
					unsigned long long b_index = Indexer::FlatIndex3(2, x_index, y_index, 3, render_size.x);

					Vector3 voxel_position{ x_index, k, y_index };
					unsigned long long voxel_index = Indexer::FlatIndex3(voxel_position.x, voxel_position.y, voxel_position.z, this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.y);
					VoxelData voxel_data = world_voxels[voxel_index];

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
			printf("\n    Saving chunk...\n");
			printf("          Chunk Time: %d s\n", time_lapsed);
		}
	}

	free(img_cpu);

	printf("Done!\n");
}

void CpuEngine::EstimatePositionIhm(GuiData gui_data) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	IhmState ihm_state = this->ihm_generator.GetIhmState(gui_data.ihm_index, false);
	Vector3 anchor = this->camera_list[0]->transform.position / this->ihm_generator.world_stride;
	anchor.x += 0;
	anchor.z += 0;
	Vector3* estimates = CudaIhm::EstimatePosition(this->ihm_cortex, this->ihm_generator, this->camera_list[0]->phash_data, anchor, ihm_state.direction_index);

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	printf("        Time Elapsed: %d s\n", time_lapsed);

	printf("\nEstimated Position0: {%.2f, %.2f, %.2f}\n", estimates[0].x, estimates[0].y, estimates[0].z);
	printf("    Error: %.2f m\n", Transform::Norm3(estimates[0] - this->camera_list[0]->transform.position) / 10);
	printf("Estimated Position1: {%.2f, %.2f, %.2f}\n", estimates[1].x, estimates[1].y, estimates[1].z);
	printf("    Error: %.2f m\n", Transform::Norm3(estimates[1] - this->camera_list[0]->transform.position) / 10);
	printf("Estimated Position1: {%.2f, %.2f, %.2f}\n", estimates[2].x, estimates[2].y, estimates[2].z);
	printf("    Error: %.2f m\n\n", Transform::Norm3(estimates[2] - this->camera_list[0]->transform.position) / 10);

	//free(estimates);
}

void CpuEngine::RenderPathConfusion(GuiData gui_data) {
	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };
	Vector3 render_size_3d{ this->world_space->space_data.world_size0.x, 1, this->world_space->space_data.world_size0.z };
	std::string save_path = "./data/results/path_confusion.jpg";

	unsigned long long byte_count = render_size_3d.x * render_size_3d.z * 3;
	byte* img_cpu = new byte[byte_count];
	memset(img_cpu, 0, byte_count);

	VoxelData* world_voxels = new VoxelData[this->world_space->space_data.voxel_count0];
	byte_count = this->world_space->space_data.voxel_count0 * sizeof(VoxelData);
	memset(world_voxels, 0, byte_count);
	CudaError::CheckError((cudaError_enum)cudaMemcpy(world_voxels, this->world_space->space_data.space0, byte_count, cudaMemcpyDeviceToHost), __FILE__, __LINE__);

	int direction_index = 12;
	for (int i = 0; i < this->node_count - 1; i++) {
		Vector3 node0 = this->nodes[i];
		Vector3 node1 = this->nodes[i + 1];

		node0.Print("Nodes: ", " ");
		node1.Print();
		printf("    Progress: ");

		Vector3 current_position = node0;

		Vector3 start_voxel = node0.Floor();
		Vector3 current_voxel = node0.Floor();
		Vector3 end_voxel = node1.Floor();

		Vector3 voxel_difference = end_voxel - start_voxel;
		Vector3 voxel_distance = voxel_difference.Absolute();
		Vector3 difference_sign = voxel_difference.Sign();

		int driving_axis = 0;
		int second_axis = 1;
		int third_axis = 2;

		if (voxel_distance.y >= voxel_distance.x && voxel_distance.y >= voxel_distance.z) {
			driving_axis = 1;
			second_axis = 0;
			third_axis = 2;
		}
		else if (voxel_distance.z >= voxel_distance.x && voxel_distance.z >= voxel_distance.y) {
			driving_axis = 2;
			second_axis = 0;
			third_axis = 1;
		}

		float second_slope = voxel_difference[second_axis] / voxel_difference[driving_axis];
		float third_slope = voxel_difference[third_axis] / voxel_difference[driving_axis];

		float second_bias = start_voxel[second_axis] - (start_voxel[driving_axis] * second_slope);
		float third_bias = start_voxel[third_axis] - (start_voxel[driving_axis] * third_slope);

		int driving_distance = 0;
		while (driving_distance <= voxel_distance[driving_axis]) {
			driving_distance++;
			current_position[driving_axis] += difference_sign[driving_axis];
			current_position[second_axis] = (current_position[driving_axis] * second_slope) + second_bias;
			current_position[third_axis] = (current_position[driving_axis] * third_slope) + third_bias;

			if (!current_position.IsBounded(Vector::ZERO3(), this->world_space->space_data.world_size0 - 1)) {
				break;
			}

			current_voxel = current_position.Floor();

			this->camera_list[0]->transform.position = current_voxel;
			this->camera_list[0]->transform.rotation = Quaternion::QuaternionFromDirection(this->ihm_generator.directions_cpu[direction_index]);
			this->RenderUpdate(gui_data);

			byte* phash_cpu = this->camera_list[0]->GetPhash();
			Vector3 anchor = this->camera_list[0]->transform.position / this->ihm_generator.world_stride;
			anchor = anchor.Floor();

			/*for (int i = 0; i < 16; i++) {
				for (int j = 0; j < 16; j++) {
					unsigned long long index = Indexer::FlatIndex2(j, i, 16);

					printf("[%d]", phash_cpu[index]);
				}
				printf("\n");
			}*/

			Vector3* estimates = CudaIhm::EstimatePosition(this->ihm_cortex, this->ihm_generator, this->camera_list[0]->phash_data, anchor, direction_index, false);
			Vector3 estimate_voxel = (estimates[0]).Floor();

			unsigned long long r_index = Indexer::FlatIndex3(0, current_voxel.x, current_voxel.z, 3, render_size.x);
			unsigned long long g_index = Indexer::FlatIndex3(1, current_voxel.x, current_voxel.z, 3, render_size.x);
			unsigned long long b_index = Indexer::FlatIndex3(2, current_voxel.x, current_voxel.z, 3, render_size.x);

			img_cpu[r_index] = 0;
			img_cpu[g_index] = 0;
			img_cpu[b_index] = 255;

			r_index = Indexer::FlatIndex3(0, estimate_voxel.x, estimate_voxel.z, 3, render_size.x);
			g_index = Indexer::FlatIndex3(1, estimate_voxel.x, estimate_voxel.z, 3, render_size.x);
			b_index = Indexer::FlatIndex3(2, estimate_voxel.x, estimate_voxel.z, 3, render_size.x);

			img_cpu[r_index] = 255;
			img_cpu[g_index] = 0;
			img_cpu[b_index] = 0;

			printf("*");
		}

		printf("\n");
	}

	int k = 40;
	for (int w = 0; w < render_size.x; w++) {
		for (int h = 0; h < render_size.y; h++) {
			Vector3 voxel_position{ w, k, h };
			unsigned long long voxel_index = Indexer::FlatIndex3(w, k, h, this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.y);
			VoxelData voxel_data = world_voxels[voxel_index];

			if (voxel_data.entity_id > 0) {
				unsigned long long r_index = Indexer::FlatIndex3(0, w, h, 3, render_size.x);
				unsigned long long g_index = Indexer::FlatIndex3(1, w, h, 3, render_size.x);
				unsigned long long b_index = Indexer::FlatIndex3(2, w, h, 3, render_size.x);

				img_cpu[r_index] = 255;
				img_cpu[g_index] = 255;
				img_cpu[b_index] = 255;
			}

		}
	}

	int result = stbi_write_jpg(save_path.c_str(), render_size.x, render_size.y, 3, img_cpu, 100);

	printf("Done!\n");
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