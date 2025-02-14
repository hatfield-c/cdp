#include "CpuEngine.h"

CpuEngine::CpuEngine(std::vector<CUdeviceptr> depth_textures, std::vector<CUdeviceptr> phash_textures, std::vector<CUdeviceptr> height_textures) {
	this->world_space = new WorldSpace();
	this->image_builder->Init();

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
		CUdeviceptr phash_derotated_texture = height_textures[i];

		Camera* camera = new Camera{};
		camera->Init("camera " + i, depth_texture, phash_texture, phash_derotated_texture);
		
		this->camera_list.push_back(camera);
	}

	Vector3 direction = Vector3{ 1, 0, 0 };

	this->drone_alpha.Init();
	this->drone_alpha.wallrider.height_texture = this->camera_list[0]->height_texture;
	this->drone_alpha.rigidbody.position = Vector3{ 90, 4, 5 };
	//this->drone_alpha.rigidbody.position = Vector3{ 24, 4, 34 };
	//this->drone_alpha.rigidbody.position = Vector3{ 48, 4, 42 };

	Vector4 x_rot = Quaternion::QuaternionFromEulerParams(Vector3{ 0, 0, 1 }, -Math::Pi() / 4);
	Vector4 quat = x_rot;// Quaternion::QuaternionFromEulerParams(Vector3{ 0, 1, 0 }, 3 * Math::Pi() / 4);
	//quat = Quaternion::MultiplyQuaternions(quat, x_rot, false);

	this->drone_alpha.rigidbody.rotation = Quaternion::QuaternionFromDirection(direction);//this->ihm_generator.directions_cpu[12]);
	//this->drone_alpha.rigidbody.rotation = quat;
	//this->drone_alpha.rigidbody.velocity = direction;
	//this->drone_alpha.rigidbody.velocity.z = -1;
	//this->drone_alpha.rigidbody.angular_velocity.y = -0.2;
	//this->drone_alpha.rigidbody.angular_velocity.z = -0.2;

	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };
	std::string save_path = "./data/results/path_confusion.jpg";

	unsigned long long byte_count = render_size.x * render_size.y * 3;
	this->simulation_image = new byte[byte_count];
	memset(this->simulation_image, 0, byte_count);
}

void CpuEngine::Start(GuiData* gui_data) {
	this->cycle_count = 0;
}

void CpuEngine::Update(GuiData* gui_data) {

	if (gui_data->is_simulating) {
		if (!gui_data->is_paused || gui_data->is_step_simulation) {
			this->ScenarioUpdate(gui_data);
			this->PhysicsUpdate(gui_data);
		}

		this->RenderUpdate(gui_data);
	}

	std::chrono::steady_clock::duration frame_time_passed = std::chrono::steady_clock::now() - this->frame_begin_time;
	unsigned long long time_lapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time_passed).count();
	unsigned long long delta_time_steps = Physics::DeltaTimeMilli();

	if (time_lapsed < delta_time_steps) {
		unsigned long long sleep_time = delta_time_steps - time_lapsed;
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
	}

	this->frame_begin_time = std::chrono::steady_clock::now();
}

void CpuEngine::End(GuiData* gui_data) {
	
}

void CpuEngine::ScenarioUpdate(GuiData* gui_data) {

	gui_data->drone_voxel = (this->drone_alpha.rigidbody.position * this->ihm_generator.world_stride).Floor();
	gui_data->drone_position = this->drone_alpha.rigidbody.position;
	gui_data->drone_forward = Quaternion::RotatePoint(Vector::FORWARD(), this->drone_alpha.rigidbody.rotation);
	gui_data->drone_quaternion = this->drone_alpha.rigidbody.rotation;
	gui_data->drone_velocity = this->drone_alpha.rigidbody.velocity;
	gui_data->drone_angular_velocity = this->drone_alpha.rigidbody.angular_velocity;
	gui_data->wallride_sensor = Vector3{
		(float)this->drone_alpha.wallrider.steer_proximity[0],
		(float)this->drone_alpha.wallrider.steer_proximity[1],
		(float)this->drone_alpha.wallrider.steer_proximity[2]
	};

	this->camera_list[0]->transform.position = this->drone_alpha.rigidbody.position * this->ihm_generator.world_stride;
	this->camera_list[0]->transform.rotation = this->drone_alpha.rigidbody.ForwardQuaternion();

	float* depth_phash = this->camera_list[0]->GetPhashAsFloat();
	Vector3* camera_cloud = this->camera_list[0]->GetCloud();
	this->drone_alpha.Update(depth_phash, camera_cloud);

	if (gui_data->control_index == 0) {
		gui_data->camera_position = this->drone_alpha.rigidbody.position;
	}
	else if (gui_data->control_index == 4) {
		gui_data->camera_position = this->drone_alpha.rigidbody.position;

		this->drone_alpha.FollowCommand(gui_data->keyboard);
	}
	else if (gui_data->control_index == 5) {
		gui_data->camera_position = this->drone_alpha.rigidbody.position;

		this->drone_alpha.Act();
	}
	else  {
		IhmState ihm_state = this->ihm_generator.GetIhmState(gui_data->ihm_index, false);

		this->camera_list[0]->transform.position = ihm_state.position;
		this->camera_list[0]->transform.rotation = ihm_state.rotation;
	}

	this->DrawDronePosition();

	/*printf(
		"[%lld] Pos:(%.2f, %.2f, %.2f) Rot:(%.2f, %.2f, %.2f, %.2f) Vel:(%.2f, %.2f, %.2f) AnV:(%.2f, %.2f, %.2f)\n",
		gui_data->ihm_index,
		this->camera_list[0]->transform.position.x, this->camera_list[0]->transform.position.y, this->camera_list[0]->transform.position.z,
		this->camera_list[0]->transform.rotation.x, this->camera_list[0]->transform.rotation.y, this->camera_list[0]->transform.rotation.z, this->camera_list[0]->transform.rotation.w,
		this->drone_alpha.rigidbody.velocity.x, this->drone_alpha.rigidbody.velocity.y, this->drone_alpha.rigidbody.velocity.z,
		this->drone_alpha.rigidbody.angular_velocity.x, this->drone_alpha.rigidbody.angular_velocity.y, this->drone_alpha.rigidbody.angular_velocity.z
	);*/
}

void CpuEngine::PhysicsUpdate(GuiData* gui_data) {
	Vector3 wind = Vector::ZERO3();

	//this->drone_alpha.rigidbody.Accelerate(Physics::Gravity());
	this->drone_alpha.rigidbody.AirResistance(wind);
	this->drone_alpha.rigidbody.Update();

	this->drone_alpha.rigidbody.position = this->drone_alpha.rigidbody.position.Clip(Vector::ZERO3(), this->ihm_generator.world_size_strided - 0.1);
}

void CpuEngine::RenderUpdate(GuiData* gui_data) {
	int camera_index = gui_data->camera_index;

	CudaCamera::RenderCamera(*this->camera_list[camera_index], this->world_space->space_data);
	cudaDeviceSynchronize();
	CudaCamera::BuildCloud(*this->camera_list[camera_index]);
	cudaDeviceSynchronize();
	this->camera_list[0]->seed++;

	// debug code
	//std::cin.ignore();
}

void CpuEngine::SaveSimulationImage(GuiData* gui_data) {
	std::string save_path = "./data/results/simulation.jpg";

	VoxelData* world_voxels = new VoxelData[this->world_space->space_data.voxel_count0];
	unsigned long long byte_count = this->world_space->space_data.voxel_count0 * sizeof(VoxelData);
	memset(world_voxels, 0, byte_count);
	CudaError::CheckError((cudaError_enum)cudaMemcpy(world_voxels, this->world_space->space_data.space0, byte_count, cudaMemcpyDeviceToHost), __FILE__, __LINE__);

	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };

	int k = 40;
	for (int w = 0; w < render_size.x; w++) {
		for (int h = 0; h < render_size.y; h++) {
			Vector3 voxel_position{ w, k, h };
			unsigned long long voxel_index = Indexer::FlatIndex3(w, k, h, this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.y);
			VoxelData voxel_data = world_voxels[voxel_index];

			if (voxel_data.entity_id > 0) {
				this->image_builder->WritePixel(this->simulation_image, render_size, Vector2{ (float)w, (float)h }, Vector3{ 255, 255, 255 }, true);
			}
		}
	}

	int result = stbi_write_jpg(save_path.c_str(), render_size.x, render_size.y, 3, this->simulation_image, 100);

	printf("Saved simulation image at: %s\n", save_path.c_str());
}

void CpuEngine::DrawDronePosition() {
	Vector3 position = this->drone_alpha.rigidbody.position * this->ihm_generator.world_stride;
	Vector2 render_position{ position.x, position.z };
	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };

	this->image_builder->WritePixel(this->simulation_image, render_size, render_position, Vector3{ 0, 0, 255 }, true);
}

void CpuEngine::GenerateIhm(GuiData* gui_data) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	
	printf("Saving IHM at path: %s\n", gui_data->save_ihm_path.c_str());

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
	fopen_s(&out_file, gui_data->save_ihm_path.c_str(), "wb");
	if (out_file == NULL) {
		printf("\n\nWarning: File did not open when saving IHM:\n    %s!\n", gui_data->save_ihm_path.c_str());
		exit(1);
	}
	int result = fwrite(ihm_cpu, sizeof(byte), memory_size, out_file);
	fclose(out_file);

	printf("    Done!\n\n");

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	int time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	printf("        Time Elapsed: %d s\n", time_lapsed);
}

void CpuEngine::LoadIhm(GuiData* gui_data) {
	printf("Loading IHM at path: %s\n", gui_data->load_ihm_path.c_str());

	unsigned long long phash_memory_size = this->ihm_generator.bit_count * sizeof(byte);
	printf("    Allocating IHM CPU memory...\n");
	printf("        Size: %.2f MB\n", phash_memory_size / 1000000.0);
	byte* ihm_cpu = new byte[phash_memory_size];

	printf("    Allocating IHM GPU memory...\n");
	byte* ihm;
	CudaError::CheckError((cudaError_enum)cudaMalloc(&ihm, phash_memory_size), __FILE__, __LINE__);

	unsigned long long cloud_memory_size = this->ihm_generator.bit_count * sizeof(Vector3);
	printf("    Allocating point cloud GPU memory...\n");
	printf("        Size: %.2f MB\n", cloud_memory_size / 1000000.0);
	Vector3* ihm_clouds;
	CudaError::CheckError((cudaError_enum)cudaMalloc(&ihm_clouds, cloud_memory_size), __FILE__, __LINE__);

	printf("    Loading IHM from disk...\n");

	FILE* in_file;
	fopen_s(&in_file, gui_data->load_ihm_path.c_str(), "rb");
	if (in_file == NULL) {
		printf("\n\nWarning: File did not open when loading IHM:\n    %s!\n", gui_data->load_ihm_path.c_str());
		exit(1);
	}
	int result = fread(ihm_cpu, sizeof(byte), phash_memory_size, in_file);
	fclose(in_file);

	printf("    Copying IHM to GPU...\n");
	CudaError::CheckError((cudaError_enum)cudaMemcpy(ihm, ihm_cpu, phash_memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);

	printf("    Extracting point cloud from IHM...\n");
	CudaIhm::ExtractRenderClouds(this->world_space->space_data, *this->camera_list[0], this->ihm_generator, ihm, ihm_clouds);

	this->ihm_cortex.Init(ihm, ihm_cpu, ihm_clouds, this->ihm_generator.direction_count);

	printf("    Done!\n\n");
}

void CpuEngine::Playground(GuiData* gui_data) {
	
}

void CpuEngine::SaveDepthPhash(GuiData* gui_data) {
	std::string save_path = gui_data->save_phash_path;
	printf("Saving Depth P-Hash at path: %s\n", save_path.c_str());

	Camera camera = *this->camera_list[0];
	
	float* proximity_hash = this->drone_alpha.wallrider.proximity_phash;
	byte* forward_phash = camera.GetPhashAsByte();
	byte* height_texture = new byte[4 * 32 * 32];

	CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaMemcpy(height_texture, this->drone_alpha.wallrider.height_texture, 4 * 32 * 32, cudaMemcpyDeviceToHost), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

	FILE* out_file;
	fopen_s(&out_file, save_path.c_str(), "wb+");
	if (out_file == NULL) {
		printf("\n\nWarning: File did not open when saving Depth P-Hash:\n    %s!\n", save_path.c_str());
		exit(1);
	}
	int result = fwrite(proximity_hash, sizeof(float), 16, out_file);
	fclose(out_file);

	result = stbi_write_jpg((save_path + ".jpg").c_str(), camera.phash_data_size.x, camera.phash_data_size.y, 1, forward_phash, 100);
	result = stbi_write_jpg((save_path + ".height.jpg").c_str(), 32, 32, 4, height_texture, 100);

	printf("    Done!\n\n");
}

void CpuEngine::SaveConfusionMap(GuiData* gui_data) {
	Vector2 render_size{ this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.z };
	Vector3 render_size_3d{ this->world_space->space_data.world_size0.x, 1, this->world_space->space_data.world_size0.z };
	Vector3 chunk_size{ 100, 1, 100 };
	Vector3 chunk_count = (render_size_3d / chunk_size).Ceil();
	Vector3 chunked_size = chunk_count * chunk_size;
	std::string save_path = "./data/results/confusion_map.jpg";

	//Vector2 chunks_lower{ 0, 0 };
	//Vector2 chunks_upper{ chunk_count.x, chunk_count.z };
	Vector2 chunks_lower{ 8, 7 };
	Vector2 chunks_upper{ 9, 8 };

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

						byte* phash_cpu = this->camera_list[0]->GetPhashAsByte();
						Vector3 anchor = this->camera_list[0]->transform.position / this->ihm_generator.world_stride;
						anchor = anchor.Floor();

						this->ihm_cortex.ResetDistanceBuffers();
						CudaIhm::UpdateChamferDistances(this->ihm_cortex, this->ihm_generator, this->camera_list[0]->render_cloud, anchor);
						this->ihm_cortex.seed = ihm_cortex.NextSample(ihm_cortex.seed);

						IhmEstimate estimate = this->ihm_cortex.EstimateIhmState();
						Vector3 offset = estimate.position - this->ihm_cortex.search_radius;
						Vector3 position = anchor + offset;

						Vector3 estimate_voxel = position * this->ihm_generator.world_stride;

						float score = Transform::Norm3(this->camera_list[0]->transform.position - estimate_voxel);
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

void CpuEngine::EstimatePositionIhm(GuiData* gui_data) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	Vector3 anchor = (this->camera_list[0]->transform.position / this->ihm_generator.world_stride).Floor();

	this->ihm_cortex.ResetDistanceBuffers();

	CudaIhm::UpdateChamferDistances(this->ihm_cortex, this->ihm_generator, this->camera_list[0]->render_cloud, anchor);
	this->ihm_cortex.seed = ihm_cortex.NextSample(ihm_cortex.seed);

	IhmEstimate estimate = this->ihm_cortex.EstimateIhmState();
	Vector3 offset = estimate.position - this->ihm_cortex.search_radius;
	Vector3 position = anchor + offset;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	int time_lapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
	
	printf("Ihm Estimate:\n");
	printf("    Score: %.2f\n", estimate.chamfer_score);
	printf("    Direction: %d\n", estimate.direction_index);
	offset.Print("    Offset: ");
	position.Print("    Position: ");
	printf("    Time Elapsed: %d ms\n", time_lapsed);
}

void CpuEngine::RenderPathConfusion(GuiData* gui_data) {
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

	ImageBuilder image_builder{};
	image_builder.Init();

	Vector3 anchor = this->camera_list[0]->transform.position / this->ihm_generator.world_stride;
	anchor = anchor.Floor();
	/*
	for (int i = 0; i < this->node_count - 1; i++) {
		Vector3 node0 = this->nodes[i];
		Vector3 node1 = this->nodes[i + 1];

		Vector3 node_direction = Transform::Unit3(node1 - node0);
		int direction_index = this->ihm_generator.GetClosestDirectionIndex(node_direction);// 12;

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
			this->camera_list[0]->transform.rotation = Quaternion::QuaternionFromDirection(node_direction);
			this->RenderUpdate(gui_data);

			//anchor = this->camera_list[0]->transform.position / this->ihm_generator.world_stride;
			//anchor = anchor.Floor();

			byte* phash_cpu = this->camera_list[0]->GetPhashAsByte();
			
			this->ihm_cortex.ResetDistanceBuffers();
			CudaIhm::UpdateChamferDistances(this->ihm_cortex, this->ihm_generator, this->camera_list[0]->render_cloud, anchor);
			this->ihm_cortex.seed = ihm_cortex.NextSample(ihm_cortex.seed);

			IhmEstimate estimate = this->ihm_cortex.EstimateIhmState();
			Vector3 offset = estimate.position - this->ihm_cortex.search_radius;
			Vector3 position = anchor + offset;

			Vector3 estimate_voxel = position * this->ihm_generator.world_stride;
			anchor = (position * (1.0 / 3.0)) + (anchor * (2.0 / 3.0));

			Vector2 current_pixel = Vector2{ current_voxel.x, current_voxel.z };
			Vector2 estimate_pixel = Vector2{ estimate_voxel.x, estimate_voxel.z };

			image_builder.DrawLine_Serial(img_cpu, render_size, Vector2{ current_voxel.x, current_voxel.z }, Vector2{ estimate_voxel.x, estimate_voxel.z }, Vector3{ 255, 0, 0 });
			image_builder.WritePixel(img_cpu, render_size, estimate_pixel, Vector3{ 0, 255, 0 });

			printf("*");
		}

		image_builder.DrawLine_Serial(img_cpu, render_size, Vector2{ end_voxel.x, end_voxel.z }, Vector2{ start_voxel.x, start_voxel.z }, Vector3{ 0, 0, 255 });

		printf("\n");
	}
	*/
	int k = 40;
	for (int w = 0; w < render_size.x; w++) {
		for (int h = 0; h < render_size.y; h++) {
			Vector3 voxel_position{ w, k, h };
			unsigned long long voxel_index = Indexer::FlatIndex3(w, k, h, this->world_space->space_data.world_size0.x, this->world_space->space_data.world_size0.y);
			VoxelData voxel_data = world_voxels[voxel_index];

			if (voxel_data.entity_id > 0) {
				unsigned long long r_index = Indexer::FlatIndex3(0, w, render_size.y - h - 1, 3, render_size.x);
				unsigned long long g_index = Indexer::FlatIndex3(1, w, render_size.y - h - 1, 3, render_size.x);
				unsigned long long b_index = Indexer::FlatIndex3(2, w, render_size.y - h - 1, 3, render_size.x);

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