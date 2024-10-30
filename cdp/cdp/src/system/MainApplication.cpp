#include "MainApplication.h"

MainApplication::MainApplication() {
    this->main_gui = new MainGui(1);

    std::vector<CUdeviceptr> camera_textures = this->main_gui->vulkan_pipeline->GetCameraTextures();

    this->engine = new CudaEngine(camera_textures);
}

void MainApplication::Run() {
    bool is_simulating = false;

    while (!this->main_gui->IsWindowClosed()) {
        this->main_gui->Update();

        bool delta = (is_simulating != this->main_gui->is_simulating);

        if (delta && !is_simulating) {
            this->engine->Initialize();
        }
        else if (!delta && is_simulating) {
            this->engine->Update();
        }
        else if(delta && is_simulating) {
            this->engine->Reset();
        }
        
        is_simulating = this->main_gui->is_simulating;
    }

    this->main_gui->Cleanup();
    this->engine->Cleanup();
}