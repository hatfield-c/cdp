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
	this->camera_list[0]->camera_data.transform.position = Vector3{ 50, 500, 80 };
	this->camera_list[0]->camera_data.transform.rotation = QuaternionFromEulerAngles(Vector3{ 0, 0, 0 });
}

void CudaEngine::Update() {
	this->ScenarioUpdate();
	this->PhysicsUpdate();
	this->RenderUpdate();
}

void CudaEngine::Reset() {

}

void CudaEngine::ScenarioUpdate() {
	this->camera_list[0]->camera_data.transform.position.x += 1;
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