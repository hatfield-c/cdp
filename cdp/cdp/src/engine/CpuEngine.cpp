#include "CpuEngine.h"

CpuEngine::CpuEngine(std::vector<CUdeviceptr> camera_textures) {

	for (int i = 0; i < camera_textures.size(); i++) {
		CUdeviceptr gpu_texture = camera_textures[i];
		Camera* camera = new Camera("camera " + i, gpu_texture);
		
		this->camera_list.push_back(camera);
	}

	// todo: Parallelize world init
	this->world_space = new WorldSpace();
}

void CpuEngine::Start() {
	this->cycle_count = 0;

	//this->camera_list[0]->camera_data.transform.position = Vector3{ 220, 40, 580};
	this->camera_list[0]->camera_data.transform.position = Vector3{ 780, 40, 300 };
	this->camera_list[0]->camera_data.transform.rotation = Quaternion::QuaternionFromEulerAngles(Vector3{ 0, 1.35, 0 });
}

void CpuEngine::Update() {
	this->ScenarioUpdate();
	this->PhysicsUpdate();
	this->RenderUpdate();
}

void CpuEngine::End() {
	
}

void CpuEngine::ScenarioUpdate() {
	Vector3 target_location{ 480, 40, 420 };

	Vector3 offset = this->camera_list[0]->camera_data.transform.position - target_location;
	Vector4 rotation_amount = Quaternion::QuaternionFromEulerAngles(Vector3{ 0, 0.003, 0 });
	Vector3 new_position = Quaternion::RotatePoint(offset, rotation_amount) + target_location;

	this->camera_list[0]->camera_data.transform.position = new_position;

	Vector4 camera_rotation = Quaternion::MultiplyQuaternions(rotation_amount, this->camera_list[0]->camera_data.transform.rotation, false);

	this->camera_list[0]->camera_data.transform.rotation = camera_rotation;
}

void CpuEngine::PhysicsUpdate() {

}

void CpuEngine::RenderUpdate() {
	for (int i = 0; i < this->camera_list.size(); i++) {
		this->camera_list[i]->Render(this->world_space);
	}
}

void CpuEngine::Cleanup() {
	printf("Cleaning CudaEngine...\n");
	this->world_space->Cleanup();
	printf("    Done!\n");
}