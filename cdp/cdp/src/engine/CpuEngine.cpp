#include "CpuEngine.h"

CpuEngine::CpuEngine(std::vector<CUdeviceptr> depth_textures, std::vector<CUdeviceptr> phash_textures, std::vector<CUdeviceptr> shaded_textures) {
	this->ihm_generator.Init(3);

	for (int i = 0; i < depth_textures.size(); i++) {
		CUdeviceptr depth_texture = depth_textures[i];
		CUdeviceptr phash_texture = phash_textures[i];
		CUdeviceptr shaded_texture = shaded_textures[i];

		Camera* camera = new Camera{};// new Camera("camera " + i, gpu_texture);
		camera->Init("camera " + i, depth_texture, phash_texture, shaded_texture);
		
		this->camera_list.push_back(camera);
	}

	this->world_space = new WorldSpace();
	
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
	Vector3 target_location{ 480, 40, 420 };

	Vector3 offset = this->camera_list[0]->transform.position - target_location;
	Vector4 rotation_amount = Quaternion::QuaternionFromEulerAngles(Vector3{ 0, 0.003, 0 });
	Vector3 new_position = Quaternion::RotatePoint(offset, rotation_amount) + target_location;

	this->camera_list[0]->transform.position = new_position;

	Vector4 camera_rotation = Quaternion::MultiplyQuaternions(rotation_amount, this->camera_list[0]->transform.rotation, false);

	this->camera_list[0]->transform.rotation = camera_rotation;
	this->camera_list[0]->vote_threshold = gui_data.vote_threshold;
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

void CpuEngine::Cleanup() {
	printf("Cleaning CudaEngine...\n");
	this->world_space->Cleanup();
	printf("    Done!\n");
}