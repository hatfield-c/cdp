#include "MainApplication.h"

MainApplication::MainApplication() {
    this->main_gui = new MainGui(1);

    std::vector<CUdeviceptr> depth_textures = this->main_gui->vulkan_pipeline->GetDepthTextures();
    std::vector<CUdeviceptr> phash_textures = this->main_gui->vulkan_pipeline->GetPhashTextures();
    std::vector<CUdeviceptr> height_textures = this->main_gui->vulkan_pipeline->GetHeightTextures();

    this->engine = new CpuEngine(depth_textures, phash_textures, height_textures);
}

void MainApplication::Run() {

    while (!this->main_gui->IsWindowClosed()) {
        this->main_gui->Update();

        this->GuiAction(this->main_gui->gui_data);
    }

    this->main_gui->Cleanup();
    this->engine->Cleanup();
}

void MainApplication::GuiAction(GuiData* gui_data) {
    bool is_state_changed = (this->engine->is_simulating != gui_data->is_simulating);

    if (!gui_data->save_phash_path.empty()) {
        this->engine->SaveProximityHash(gui_data);
    }

    if (!gui_data->load_env_path.empty()) {
        this->engine->world_space->LoadWorld(gui_data->load_env_path);
    }

    if (gui_data->is_playground) {
        this->engine->Playground(gui_data);
    }
    if (gui_data->is_save_simulation_image) {
        this->engine->SaveSimulationImage(gui_data);
    }

    if (gui_data->is_stochastic_subtraction) {
        this->engine->world_space->ActivateStochasticSubtraction();
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

    this->engine->is_simulating = gui_data->is_simulating;
}