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
	this->drone_alpha.rigidbody.position = Vector3{ 88, 0.4, 5 };
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
	this->camera_list[0]->is_rotation_noise = gui_data->is_rotation_noise;

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
	this->drone_alpha.Drift();
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

void CpuEngine::Playground(GuiData* gui_data) {
	
}

void CpuEngine::SaveProximityHash(GuiData* gui_data) {
	std::string save_path = gui_data->save_phash_path;
	printf("Saving Proximity Hash at path: %s\n", save_path.c_str());

	Camera camera = *this->camera_list[0];
	
	float* proximity_hash = this->drone_alpha.wallrider.proximity_blur;
	byte* forward_phash = camera.GetPhashAsByte();
	byte* height_texture = new byte[4 * 32 * 32];

	CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaMemcpy(height_texture, this->drone_alpha.wallrider.height_texture, 4 * 32 * 32, cudaMemcpyDeviceToHost), __FILE__, __LINE__);
	CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

	FILE* out_file;
	fopen_s(&out_file, save_path.c_str(), "wb+");
	if (out_file == NULL) {
		printf("\n\nWarning: File did not open when saving Proximity Hash:\n    %s!\n", save_path.c_str());
		exit(1);
	}
	int result = fwrite(proximity_hash, sizeof(float), 16, out_file);
	fclose(out_file);

	result = stbi_write_jpg((save_path + ".camera.jpg").c_str(), camera.phash_data_size.x, camera.phash_data_size.y, 1, forward_phash, 100);
	result = stbi_write_jpg((save_path + ".height.jpg").c_str(), camera.phash_texture_size.x, camera.phash_texture_size.x, 4, height_texture, 100);

	printf("    Done!\n\n");
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