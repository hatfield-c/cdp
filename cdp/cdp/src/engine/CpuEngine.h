#pragma once

#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <thread>
#include "cuda.h"
#include "stb_image_write.h"

#include "Transform.h"
#include "Quaternion.h"
#include "WorldSpace.h"
#include "ImageBuilder.h"

#include "../ui/GuiData.h"

#include "../entity/camera/Camera.h"
#include "../entity/camera/cuda/CudaCamera.cuh"
#include "../entity/DroneAlpha.h"
#include "../entity/WindGenerator.h"
#include "../entity/ai/hitpoly/NeuralGrid.h"
#include "../entity/ai/hitpoly/cuda/CudaHitpoly.cuh"
#include "../entity/ai/nav/PhmState.h"
#include "../entity/ai/nav/NavGenerator.h"
#include "../entity/ai/nav/cuda/CudaNavHash.cuh"

struct CpuEngine {
	bool is_simulating = false;
	int cycle_count = 0;
	std::chrono::steady_clock::time_point frame_begin_time = std::chrono::steady_clock::now();
	std::vector<Camera*> camera_list{};
	WorldSpace world_space;
	ImageBuilder* image_builder;
	NavGenerator nav_generator{};
	DroneAlpha drone_alpha{};
	WindGenerator wind_generatior{};
	byte* simulation_image;

	void Init(std::vector<CUdeviceptr> depth_textures, std::vector<CUdeviceptr> phash_textures, std::vector<CUdeviceptr> height_textures) {
		this->world_space.Init();
		this->image_builder->Init();

		this->nav_generator.Init(
			3,
			Vector2{ -Math::Pi() / 4.0f, 0.0f },
			Vector::ZERO3(),
			this->world_space.space_data.world_size0,
			this->world_space.space_data.world_size0,
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
		this->drone_alpha.rigidbody.position = Vector3{ 88.0f, 0.4f, 5.0f };
		//this->drone_alpha.rigidbody.position = Vector3{ 24, 4, 34 };
		//this->drone_alpha.rigidbody.position = Vector3{ 48, 4, 42 };

		float rot_angle = -Math::Pi() / 4;
		rot_angle = -(45.0f / 180.0f) * Math::Pi();
		Vector4 x_rot = Quaternion::QuaternionFromEulerParams(Vector3{ 0, 0, 1 }, rot_angle);
		Vector4 quat = Quaternion::QuaternionFromEulerParams(Vector3{ 1, 0, 0 }, -Math::Pi() / 4);//x_rot;
		//quat = Quaternion::MultiplyQuaternions(quat, x_rot, false);

		this->drone_alpha.rigidbody.rotation = Quaternion::QuaternionFromDirection(direction);//this->ihm_generator.directions_cpu[12]);
		this->drone_alpha.rigidbody.rotation = quat;
		//this->drone_alpha.rigidbody.velocity = direction;
		//this->drone_alpha.rigidbody.velocity.z = -1;
		//this->drone_alpha.rigidbody.angular_velocity.y = -0.2;
		//this->drone_alpha.rigidbody.angular_velocity.z = -0.2;

		Vector2 render_size{ this->world_space.space_data.world_size0.x, this->world_space.space_data.world_size0.z };
		std::string save_path = "./data/results/path_confusion.jpg";

		unsigned long long byte_count = (unsigned long long)(render_size.x * render_size.y * 3);
		this->simulation_image = new byte[byte_count];
		memset(this->simulation_image, 0, byte_count);
	}

	void Start(GuiData* gui_data) {
		this->cycle_count = 0;
	}

	void Update(GuiData* gui_data) {

		if (gui_data->is_simulating) {
			if (!gui_data->is_paused || gui_data->is_step_simulation) {
				this->ScenarioUpdate(gui_data);
				this->PhysicsUpdate(gui_data);
			}

			this->RenderUpdate(gui_data);
		}

		std::chrono::steady_clock::duration frame_time_passed = std::chrono::steady_clock::now() - this->frame_begin_time;
		unsigned long long time_lapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time_passed).count();
		unsigned long long delta_time_steps = (unsigned long long)Physics::DeltaTimeMilli();

		if (time_lapsed < delta_time_steps) {
			unsigned long long sleep_time = delta_time_steps - time_lapsed;
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
		}

		this->frame_begin_time = std::chrono::steady_clock::now();
	}

	void End(GuiData* gui_data) {

	}

	void ScenarioUpdate(GuiData* gui_data) {
		this->camera_list[0]->is_rotation_noise = gui_data->is_rotation_noise;

		gui_data->drone_voxel = (this->drone_alpha.rigidbody.position * this->nav_generator.world_stride).Floor();
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

		this->camera_list[0]->transform.position = this->drone_alpha.rigidbody.position * this->nav_generator.world_stride;
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
		else {
			PhmState ihm_state = this->nav_generator.GetPhmState(gui_data->ihm_index, false);

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

	void PhysicsUpdate(GuiData* gui_data) {
		Vector3 wind = this->wind_generatior.RandomWind() * 0;

		//this->drone_alpha.rigidbody.Accelerate(Physics::Gravity());
		//this->drone_alpha.Drift();
		this->drone_alpha.rigidbody.AirResistance(wind);
		this->drone_alpha.rigidbody.Update();

		this->drone_alpha.rigidbody.position = this->drone_alpha.rigidbody.position.Clip(Vector::ZERO3(), this->nav_generator.world_size_strided - 0.1f);
	}

	void RenderUpdate(GuiData* gui_data) {
		int camera_index = gui_data->camera_index;

		CudaCamera::RenderCamera(*this->camera_list[camera_index], this->world_space.space_data);
		cudaDeviceSynchronize();
		CudaCamera::BuildCloud(*this->camera_list[camera_index]);
		cudaDeviceSynchronize();
		this->camera_list[0]->seed++;

		// debug code
		//std::cin.ignore();
	}

	void SaveSimulationImage(GuiData* gui_data) {
		std::string save_path = "./data/results/simulation.jpg";

		VoxelData* world_voxels = new VoxelData[this->world_space.space_data.voxel_count0];
		unsigned long long byte_count = this->world_space.space_data.voxel_count0 * sizeof(VoxelData);
		memset(world_voxels, 0, byte_count);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(world_voxels, this->world_space.space_data.space0, byte_count, cudaMemcpyDeviceToHost), __FILE__, __LINE__);

		Vector2 render_size{ this->world_space.space_data.world_size0.x, this->world_space.space_data.world_size0.z };

		int k = 40;
		for (int w = 0; w < render_size.x; w++) {
			for (int h = 0; h < render_size.y; h++) {
				Vector3 voxel_position{ (float)w, (float)k, (float)h };
				unsigned long long voxel_index = Indexer::FlatIndex3((float)w, (float)k, (float)h, this->world_space.space_data.world_size0.x, this->world_space.space_data.world_size0.y);
				VoxelData voxel_data = world_voxels[voxel_index];

				if (voxel_data.entity_id > 0) {
					this->image_builder->WritePixel(this->simulation_image, render_size, Vector2{ (float)w, (float)h }, Vector3{ 255, 255, 255 }, true);
				}
			}
		}

		int result = stbi_write_jpg(save_path.c_str(), (int)render_size.x, (int)render_size.y, 3, this->simulation_image, 100);

		printf("Saved simulation image at: %s\n", save_path.c_str());
	}

	void DrawDronePosition() {
		Vector3 position = this->drone_alpha.rigidbody.position * this->nav_generator.world_stride;
		Vector2 render_position{ position.x, position.z };
		Vector2 render_size{ this->world_space.space_data.world_size0.x, this->world_space.space_data.world_size0.z };

		this->image_builder->WritePixel(this->simulation_image, render_size, render_position, Vector3{ 0, 0, 255 }, true);
	}

	void GenerateHitPolyData(GuiData* gui_data) {
		CudaHitPoly::GenerateTrainingData();
	}

	void GeneratePhm(GuiData* gui_data) {
		std::chrono::steady_clock::time_point frame_begin_time = std::chrono::steady_clock::now();

		NavGenerator slice_generator{};
		slice_generator.Init(
			3,
			Vector2{ -Math::Pi() / 4.0f, 0.0f },
			Vector3{ 0, 50, 0 },
			Vector3{ 1000, 30, 1000 },
			this->world_space.space_data.world_size0,
			Vector3{ 10, 10, 10 },
			Vector2{ 16, 16 }
		);

		float* phm;
		cudaMalloc(&phm, slice_generator.bit_count * sizeof(float));
		cudaMemset(phm, 0, slice_generator.bit_count * sizeof(float));
		CudaNavHash::GeneratePhm(this->world_space.space_data, *this->camera_list[0], slice_generator, phm);

		float* phm_cpu = new float[slice_generator.bit_count];
		memset(phm_cpu, 0, slice_generator.bit_count * sizeof(float));
		CudaError::CheckError((cudaError_enum)cudaMemcpy(phm_cpu, phm, slice_generator.bit_count * sizeof(float), cudaMemcpyDeviceToHost), __FILE__, __LINE__);

		printf("    Saving PHM...\n");
		std::string phm_path = "data/nav/phm/phm.float";

		FILE* ihm_file;
		fopen_s(&ihm_file, phm_path.c_str(), "wb+");
		if (ihm_file == NULL) {
			printf("\n\nWarning: File did not open when saving IHM:\n    %s!\n", phm_path.c_str());
			exit(1);
		}
		int result = (int)fwrite(phm_cpu, sizeof(float), slice_generator.bit_count, ihm_file);
		fclose(ihm_file);

		for (int i = 0; i < 157; i++) {
			unsigned long long index = i * (int)(slice_generator.state_count / 157);
			PhmState phm_state = slice_generator.GetPhmState(index, false);

			std::string img_path = "data/nav/phm/"
				+ std::to_string(index)
				+ "-"
				+ std::to_string((int)phm_state.position.x) + "_"
				+ std::to_string((int)phm_state.position.y) + "_"
				+ std::to_string((int)phm_state.position.z)
				+ ""
				+ "-"
				+ std::to_string(phm_state.direction_index)
				+ ".jpg"
				;

			float* depth_frame = new float[slice_generator.phash_count];
			byte* depth_img = new byte[slice_generator.phash_count];

			for (unsigned long long j = 0; j < 16; j++) {
				for (unsigned long long k = 0; k < 16; k++) {
					unsigned long long bit_index = Indexer::FlatIndex3(k, j, index, 16, 16);
					unsigned long long phash_index = Indexer::FlatIndex2(k, j, 16);

					float depth_value = phm_cpu[bit_index];
					float depth_float = depth_value;
					depth_float = 20.0f - depth_float;
					depth_float = depth_float / 20.0f;
					depth_float = 255.0f * depth_float;
					byte pixel_value = (byte)depth_float;

					depth_frame[phash_index] = depth_value;
					depth_img[phash_index] = pixel_value;
				}
			}

			stbi_write_jpg(img_path.c_str(), 16, 16, 1, depth_img, 100);
		}
		printf("        Done!");

		std::chrono::steady_clock::duration frame_time_passed = std::chrono::steady_clock::now() - frame_begin_time;
		unsigned long long time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(frame_time_passed).count();

		printf("\nTime elapsed: %lld s\n", time_lapsed);
	}

	void GenerateShm(GuiData* gui_data) {
		std::chrono::steady_clock::time_point frame_begin_time = std::chrono::steady_clock::now();

		NavGenerator slice_generator{};
		slice_generator.Init(
			3,
			Vector2{ -Math::Pi() / 4.0f, 0.0f },
			Vector3{ 0, 50, 0 },
			Vector3{ 1000, 30, 1000 },
			this->world_space.space_data.world_size0,
			Vector3{ 10, 10, 10 },
			Vector2{ 16, 16 }
		);

		float* ihm;
		cudaMalloc(&ihm, slice_generator.bit_count * sizeof(float));
		cudaMemset(ihm, 0, slice_generator.bit_count * sizeof(float));
		CudaNavHash::GeneratePhm(this->world_space.space_data, *this->camera_list[0], slice_generator, ihm);

		float* shm;
		cudaMalloc(&shm, slice_generator.state_count * 10 * 10 * sizeof(float));
		cudaMemset(shm, 0, slice_generator.state_count * 10 * 10 * sizeof(float));
		CudaNavHash::GenerateShm(this->world_space.space_data, slice_generator, ihm, shm);

		float* ihm_cpu = new float[slice_generator.bit_count];
		memset(ihm_cpu, 0, slice_generator.bit_count * sizeof(float));
		CudaError::CheckError((cudaError_enum)cudaMemcpy(ihm_cpu, ihm, slice_generator.bit_count * sizeof(float), cudaMemcpyDeviceToHost), __FILE__, __LINE__);

		float* shm_cpu = new float[slice_generator.state_count * 10 * 10];
		memset(shm_cpu, 0, slice_generator.state_count * 10 * 10 * sizeof(float));
		CudaError::CheckError((cudaError_enum)cudaMemcpy(shm_cpu, shm, slice_generator.state_count * 10 * 10 * sizeof(float), cudaMemcpyDeviceToHost), __FILE__, __LINE__);

		printf("    Saving IHM...\n");
		std::string save_path = "data/polyfield/training/ihm.float";
		std::string shm_path = "data/polyfield/training/shm.float";

		FILE* ihm_file;
		fopen_s(&ihm_file, save_path.c_str(), "wb+");
		if (ihm_file == NULL) {
			printf("\n\nWarning: File did not open when saving IHM:\n    %s!\n", save_path.c_str());
			exit(1);
		}
		int result = (int)fwrite(ihm_cpu, sizeof(float), slice_generator.bit_count, ihm_file);
		fclose(ihm_file);

		FILE* shm_file;
		fopen_s(&shm_file, shm_path.c_str(), "wb+");
		if (shm_file == NULL) {
			printf("\n\nWarning: File did not open when saving SHM:\n    %s!\n", shm_path.c_str());
			exit(1);
		}
		result = (int)fwrite(shm_cpu, sizeof(float), slice_generator.state_count * 10 * 10, shm_file);
		fclose(shm_file);

		for (int i = 0; i < 157; i++) {
			unsigned long long index = i * (int)(slice_generator.state_count / 157);
			PhmState ihm_state = slice_generator.GetPhmState(index, false);

			std::string img_path = "data/polyfield/training/"
				+ std::to_string(index)
				+ "-"
				+ std::to_string((int)ihm_state.position.x) + "_"
				+ std::to_string((int)ihm_state.position.y) + "_"
				+ std::to_string((int)ihm_state.position.z)
				+ ""
				+ "-"
				+ std::to_string(ihm_state.direction_index)
				+ ".jpg"
				;

			float* depth_frame = new float[slice_generator.phash_count];
			byte* depth_img = new byte[slice_generator.phash_count];

			for (unsigned long long j = 0; j < 16; j++) {
				for (unsigned long long k = 0; k < 16; k++) {
					unsigned long long bit_index = Indexer::FlatIndex3(k, j, index, 16, 16);
					unsigned long long phash_index = Indexer::FlatIndex2(k, j, 16);

					float depth_value = ihm_cpu[bit_index];
					float depth_float = depth_value;
					depth_float = 20.0f - depth_float;
					depth_float = depth_float / 20.0f;
					depth_float = 255.0f * depth_float;
					byte pixel_value = (byte)depth_float;

					depth_frame[phash_index] = depth_value;
					depth_img[phash_index] = pixel_value;
				}
			}

			byte* shm_img = new byte[10 * 10];

			for (unsigned long long j = 0; j < 10; j++) {
				for (unsigned long long k = 0; k < 10; k++) {
					unsigned long long bit_index = Indexer::FlatIndex3(k, j, index, 10, 10);
					unsigned long long phash_index = Indexer::FlatIndex2(k, j, 10);

					float s_value = shm_cpu[bit_index];

					if (index == 55020) {
						//printf("%.8f\n", s_value);
					}

					s_value = 255.0f * s_value;
					byte pixel_value = (byte)s_value;

					shm_img[phash_index] = pixel_value;
				}
			}

			//printf("        Saving image at index: %lld\n", index);
			stbi_write_jpg(img_path.c_str(), 16, 16, 1, depth_img, 100);
			stbi_write_jpg((img_path + "_shm.jpg").c_str(), 10, 10, 1, shm_img, 100);
		}
		printf("        Done!");

		std::chrono::steady_clock::duration frame_time_passed = std::chrono::steady_clock::now() - frame_begin_time;
		unsigned long long time_lapsed = std::chrono::duration_cast<std::chrono::seconds>(frame_time_passed).count();

		printf("\nTime elapsed: %lld s\n", time_lapsed);
	}

	void Playground(GuiData* gui_data) {
		std::string shm_path = "data/polyfield/training/shm.float";

		NavGenerator slice_generator{};
		slice_generator.Init(
			3,
			Vector2{ -Math::Pi() / 4.0f, 0.0f },
			Vector3{ 0, 50, 0 },
			Vector3{ 1000, 30, 1000 },
			this->world_space.space_data.world_size0,
			Vector3{ 10, 10, 10 },
			Vector2{ 16, 16 }
		);

		float* shm_cpu = new float[slice_generator.state_count * 10 * 10];

		FILE* in_file;
		fopen_s(&in_file, shm_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", shm_path.c_str());
			exit(1);
		}
		int result = (int)fread(shm_cpu, sizeof(float), (size_t)(slice_generator.state_count * 10 * 10), in_file);
		fclose(in_file);

		//for (int i = 0; i < 256; i++) {
		float s = shm_cpu[0];
		printf("%.2f - ", s);
		unsigned char* b = (unsigned char*)&s;

		for (int j = 0; j < 4; j++) {
			printf("%02x", b[j]);
		}
		printf("\n");
	}
	//}

	void SaveProximityHash(GuiData* gui_data) {
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
		int result = (int)fwrite(proximity_hash, sizeof(float), (unsigned int)16, out_file);
		fclose(out_file);

		result = stbi_write_jpg((save_path + ".camera.jpg").c_str(), (int)camera.phash_data_size.x, (int)camera.phash_data_size.y, 1, forward_phash, 100);
		result = stbi_write_jpg((save_path + ".height.jpg").c_str(), (int)camera.phash_texture_size.x, (int)camera.phash_texture_size.x, 4, height_texture, 100);

		printf("    Done!\n\n");
	}

	void Cleanup() {
		printf("Cleaning CudaEngine...\n");
		this->world_space.Cleanup();
		printf("    Done!\n");
	}
};