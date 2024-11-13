#include "MainApplication.h"

MainApplication::MainApplication() {
    this->main_gui = new MainGui(1);

    std::vector<CUdeviceptr> camera_textures = this->main_gui->vulkan_pipeline->GetCameraTextures();

    this->engine = new CpuEngine(camera_textures);
}

void MainApplication::Run() {

    while (!this->main_gui->IsWindowClosed()) {
        this->main_gui->Update();

        this->GuiAction(this->main_gui->gui_data);
    }

    this->main_gui->Cleanup();
    this->engine->Cleanup();
}

void MainApplication::GuiAction(GuiData gui_data) {
    bool is_state_changed = (this->engine->is_simulating != this->main_gui->gui_data.is_simulating);

    if (!gui_data.load_path.empty()) {
        this->engine->world_space->LoadWorld(gui_data.load_path);
    }

    if (is_state_changed && !this->engine->is_simulating) {
        this->engine->Start(gui_data);
    }
    else if (!is_state_changed && this->engine->is_simulating) {
        this->engine->Update(gui_data);
    }
    else if (is_state_changed && this->engine->is_simulating) {
        this->engine->End(gui_data);
    }

    this->engine->is_simulating = this->main_gui->gui_data.is_simulating;
}