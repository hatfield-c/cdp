#include "MainApplication.h"

MainApplication::MainApplication() {
    this->main_gui = new MainGui(1);

    std::vector<CUdeviceptr> depth_textures = this->main_gui->vulkan_pipeline->GetDepthTextures();
    std::vector<CUdeviceptr> phash_textures = this->main_gui->vulkan_pipeline->GetPhashTextures();
    std::vector<CUdeviceptr> shaded_textures = this->main_gui->vulkan_pipeline->GetShadedTextures();

    this->engine = new CpuEngine(depth_textures, phash_textures, shaded_textures);
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

    if (!gui_data.load_env_path.empty()) {
        this->engine->world_space->LoadWorld(gui_data.load_env_path);
    }

    if (!gui_data.load_ihm_path.empty()) {
        this->engine->LoadIhm(gui_data);
    }

    if (!gui_data.save_ihm_path.empty()) {
        this->engine->GenerateIhm(gui_data);
    }

    if (gui_data.is_verify_ihm) {
        this->engine->VerifyIhm(gui_data);
    }

    if (gui_data.is_save_heatmap) {
        this->engine->SaveSimilarityHeatMap(gui_data);
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