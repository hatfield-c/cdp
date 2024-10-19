#include "MainApplication.h"

MainApplication::MainApplication() {
    this->main_gui = new MainGui(3);

    std::vector<CUdeviceptr> camera_textures = this->main_gui->vulkan_pipeline->GetCameraTextures();

    this->engine = new CudaEngine(camera_textures);
}

void MainApplication::Run() {
    while (!this->main_gui->IsWindowClosed()) {
        this->main_gui->Update();
        this->engine->Update();
    }

    this->main_gui->Cleanup();
    this->engine->Cleanup();
}