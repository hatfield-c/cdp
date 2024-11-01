#include "CudaEngine.h"

CudaEngine::CudaEngine(std::vector<CUdeviceptr> camera_textures) {

	for (int i = 0; i < camera_textures.size(); i++) {
		CUdeviceptr gpu_texture = camera_textures[i];
		Camera* camera = new Camera("camera " + i, gpu_texture);
		
		this->camera_list.push_back(camera);
	}

	this->world_space = new WorldSpace();
}

void CudaEngine::Initialize() {
	//printf("(%.2f, %.2f, %.2f)\n", this->camera_list[0]->camera_data.transform.position.x, this->camera_list[0]->camera_data.transform.position.y, this->camera_list[0]->camera_data.transform.position.z);

	this->camera_list[0]->camera_data.transform.position = Vector3{ 250, 500, 80 };
	//this->camera_list[0]->camera_data.transform.rotation = QuaternionFromEulerAngles(Vector3{ 0, 0, 0 });
}

void CudaEngine::Update() {
	this->ScenarioUpdate();
	this->PhysicsUpdate();
	this->RenderUpdate();
}

void CudaEngine::Reset() {
	this->Initialize();

	this->cycle_count = 0;
}

void CudaEngine::ScenarioUpdate() {
	//this->camera_list[0]->camera_data.transform.position.x += 1;

	Vector3 offset = this->camera_list[0]->camera_data.transform.position - Vector3{ 500, 500, 80 };
	//Vector4 rotation_amount = QuaternionFromEulerAngles(Vector3{ 0, 0, 0.1 });
	//Vector3 new_offset = RotatePoint(offset, rotation_amount);
}

void CudaEngine::PhysicsUpdate() {

}

void CudaEngine::RenderUpdate() {
	for (int i = 0; i < this->camera_list.size(); i++) {
		this->camera_list[i]->Render(this->world_space);
	}
}

void CudaEngine::Cleanup() {
	printf("Cleaning CudaEngine...\n");
	this->world_space->Cleanup();
	printf("    Done!\n");
}