
#include "MainGui.h"

MainGui::MainGui(int camera_count) {
    this->load_env_dialog.SetTitle("Load Environment");
    this->save_env_dialog.SetTitle("Save Environment");
    this->load_ihm_dialog.SetTitle("Load IHM File");
    this->save_ihm_dialog.SetTitle("IHM Save Location");
    this->load_env_dialog.SetTypeFilters({ ".ply" });
    this->save_env_dialog.SetTypeFilters({ ".ply" });
    this->load_ihm_dialog.SetTypeFilters({ ".ihm" });
    this->save_ihm_dialog.SetTypeFilters({ ".ihm" });

    this->camera_count = camera_count;

    for (int i = 0; i < this->camera_count; i++) {
        std::string camera_label = "Camera " + std::to_string(i);
        this->camera_labels.push_back(camera_label);
    }

    this->vulkan_pipeline = new VulkanPipeline(this->camera_count);

    this->ihm_generator.Init(
        3,
        Vector::ZERO3(),
        Vector3{ 1000, 100, 1000 },
        Vector3{ 1000, 100, 1000 },
        Vector3{ 10, 10, 10 },
        Vector2{ 16, 16 }
    );
    this->gui_data.ihm_index = Indexer::FlatIndex4(
        this->gui_data.camera_rotation_index, 
        this->gui_data.camera_position.x, 
        this->gui_data.camera_position.y,
        this->gui_data.camera_position.z,
        this->ihm_generator.direction_count, 
        this->ihm_generator.world_width_strided.x, 
        this->ihm_generator.world_width_strided.y
    );
}

void MainGui::Update() {
    bool is_renderable = this->vulkan_pipeline->Update();

    if (!is_renderable) {
        return;
    }
    
    this->DrawBackground();
    this->DrawViewport();
    this->DrawInspector();
    //bool show_demo_window = true;
    //ImGui::ShowDemoWindow(&show_demo_window);
    
    this->load_env_dialog.Display();
    this->save_env_dialog.Display();
    this->load_ihm_dialog.Display();
    this->save_ihm_dialog.Display();

    this->RefreshGuiData();

    this->vulkan_pipeline->Render();
}

void MainGui::RefreshGuiData() {
    this->gui_data.is_window_open = !this->IsWindowClosed();

    this->gui_data.camera_index = this->camera_index;

    if (this->load_env_dialog.HasSelected()) {
        this->gui_data.load_env_path = this->load_env_dialog.GetSelected().string();
        this->load_env_dialog.ClearSelected();
    } else {
        this->gui_data.load_env_path = "";
    }

    if (this->save_env_dialog.HasSelected()) {
        this->gui_data.save_env_path = this->save_env_dialog.GetSelected().string();
        this->save_env_dialog.ClearSelected();
    } else {
        this->gui_data.save_env_path = "";
    }

    if (this->load_ihm_dialog.HasSelected()) {
        this->gui_data.load_ihm_path = this->load_ihm_dialog.GetSelected().string();
        this->load_ihm_dialog.ClearSelected();
    }
    else {
        this->gui_data.load_ihm_path = "";
    }

    if (this->save_ihm_dialog.HasSelected()) {
        this->gui_data.save_ihm_path = this->save_ihm_dialog.GetSelected().string();
        this->save_ihm_dialog.ClearSelected();
    }
    else {
        this->gui_data.save_ihm_path = "";
    }
}

void MainGui::DrawBackground() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(this->vulkan_pipeline->vulkan_core->io->DisplaySize.x, this->vulkan_pipeline->vulkan_core->io->DisplaySize.y));
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGuiWindowFlags window_settings = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize 
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground 
        | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration;
    bool is_open;

    float uv_start = this->uv_offset;
    float uv_end = this->uv_offset + 1.5f;

    ImVec2 uv0 = ImVec2(uv_start, uv_start);
    ImVec2 uv1 = ImVec2(uv_end, uv_end);

    this->uv_offset += this->uv_delta;

    if (this->uv_offset >= 1000.0f) {
        this->uv_offset = 0;
    }

    ImGui::Begin("Background", &is_open, window_settings);
    ImGui::Image((ImTextureID)this->vulkan_pipeline->texture_list[0]->instance_descriptor, ImVec2(this->vulkan_pipeline->vulkan_core->io->DisplaySize.x, this->vulkan_pipeline->vulkan_core->io->DisplaySize.y), uv0, uv1);
    ImGui::End();

    ImGui::PopStyleVar(2);
}

void MainGui::DrawViewport() {
    ImGuiWindowFlags window_settings = ImGuiWindowFlags_MenuBar;
    bool is_open;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Once);

    ImGui::Begin("Render Viewport", &is_open, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()){
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Environment")) {
                this->load_env_dialog.Open();
            }

            if (ImGui::MenuItem("Save Environment")) {
                this->save_env_dialog.Open();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Generate IHM")) {
                this->save_ihm_dialog.Open();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Load IHM")) {
                this->load_ihm_dialog.Open();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    

    ImVec2 img_size = ImGui::GetContentRegionAvail();
    ImTextureID image_texture = (ImTextureID)this->vulkan_pipeline->texture_list[1]->instance_descriptor;

    if(this->gui_data.is_simulating and this->camera_count > 0) {
        if (this->render_texture == 0) {
            image_texture = (ImTextureID)this->vulkan_pipeline->depth_textures[this->camera_index]->instance_descriptor;
        }
        else if (this->render_texture == 1) {
            image_texture = (ImTextureID)this->vulkan_pipeline->phash_textures[this->camera_index]->instance_descriptor;
        }
        else if (this->render_texture == 2) {
            image_texture = (ImTextureID)this->vulkan_pipeline->shaded_textures[this->camera_index]->instance_descriptor;
        }
    }
    
    ImGui::Image(image_texture, img_size);

    ImGui::End();
    ImGui::PopStyleVar(1);
}

void MainGui::DrawInspector() {
    ImGui::SetNextWindowSize(ImVec2(300, 660), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(940, 30), ImGuiCond_Once);

    ImGui::Begin("Inspector");

    if (!ImGui::CollapsingHeader("Metadata")) {
        ImGui::Text("[ms/F]: %.2f", 1000.0f / this->vulkan_pipeline->vulkan_core->io->Framerate);
        ImGui::Text("[FP/s]: %.1f", this->vulkan_pipeline->vulkan_core->io->Framerate);
        ImGui::Text("[X, Y]: %.1f %.1f", this->vulkan_pipeline->vulkan_core->io->MousePos[0], this->vulkan_pipeline->vulkan_core->io->MousePos[1]);
    }

    if (!ImGui::CollapsingHeader("Simulation")) {
        this->ToggleButton("is_simulating", "Run Simulation", &this->gui_data.is_simulating);
        this->DrawCameraSelector();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Render Texture");
        ImGui::SameLine();
        ImGui::Combo("##", &this->render_texture, "Depth\0P-Hash\0Shaded\0\0");
    }

    if (!ImGui::CollapsingHeader("Camera")) {
        int min_val = 0;
        int max_val = 8;

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Control Method");
        ImGui::SameLine();
        const char* selector_options[] = { "None", "IHM Index", "Camera State", "Keyboard" };
        ImGui::Combo("##control_selector", &this->gui_data.control_index, selector_options, IM_ARRAYSIZE(selector_options));

        ImGui::Separator();

        unsigned long long ihm_index = this->gui_data.ihm_index;

        float camera_position[3] = { this->gui_data.camera_position.x, this->gui_data.camera_position.y, this->gui_data.camera_position.z };
        int direction_index = this->gui_data.camera_rotation_index;
        ImGuiInputTextFlags_ ihm_index_flag = ImGuiInputTextFlags_None;
        ImGuiInputTextFlags_ camera_state_flag = ImGuiInputTextFlags_None;
        
        if (this->gui_data.control_index == 0) {
            ihm_index_flag = ImGuiInputTextFlags_ReadOnly;
            camera_state_flag = ImGuiInputTextFlags_ReadOnly;
        }
        else if (this->gui_data.control_index == 1) {
            camera_state_flag = ImGuiInputTextFlags_ReadOnly;
            IhmState ihm_state = this->ihm_generator.GetIhmState(ihm_index, false);

            camera_position[0] = ihm_state.position_strided.x;
            camera_position[1] = ihm_state.position_strided.y;
            camera_position[2] = ihm_state.position_strided.z;
            direction_index = ihm_state.direction_index;
        }
        else if (this->gui_data.control_index == 2) {
            ihm_index_flag = ImGuiInputTextFlags_ReadOnly;
            ihm_index = Indexer::FlatIndex4(direction_index, camera_position[0], camera_position[1], camera_position[2], this->ihm_generator.direction_count, this->ihm_generator.world_width_strided.x, this->ihm_generator.world_width_strided.y);
        }
        else if (this->gui_data.control_index == 3) {
            ihm_index_flag = ImGuiInputTextFlags_ReadOnly;
            camera_state_flag = ImGuiInputTextFlags_ReadOnly;

            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_1))) {
                camera_position[0]++;
            }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_2))) {
                camera_position[0]--;
            }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Q))) {
                camera_position[1]++;
            }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W))) {
                camera_position[1]--;
            }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A))) {
                camera_position[2]++;
            }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S))) {
                camera_position[2]--;
            }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
                direction_index++;
            }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X))) {
                direction_index--;
            }

            camera_position[0] = Math::Clip(camera_position[0], this->ihm_generator.world_origin.x, this->ihm_generator.world_width_strided.x - 1);
            camera_position[1] = Math::Clip(camera_position[1], this->ihm_generator.world_origin.y, this->ihm_generator.world_width_strided.y - 1);
            camera_position[2] = Math::Clip(camera_position[2], this->ihm_generator.world_origin.z, this->ihm_generator.world_width_strided.z - 1);
            direction_index = Math::Clip(direction_index, 0, this->ihm_generator.direction_count - 1);

            ihm_index = Indexer::FlatIndex4(direction_index, camera_position[0], camera_position[1], camera_position[2], this->ihm_generator.direction_count, this->ihm_generator.world_width_strided.x, this->ihm_generator.world_width_strided.y);
        }

        Vector3 direction = this->ihm_generator.directions_cpu[direction_index];
        float camera_direction[3] = { direction.x, direction.y, direction.z };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("IHM Index");
        ImGui::SameLine();
        unsigned long long one_val = 1;
        ImGui::InputScalar("##ihm_index", ImGuiDataType_U64, &ihm_index, &one_val, NULL, NULL, ihm_index_flag);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Camera Position");
        ImGui::SameLine();
        ImGui::InputFloat3("##camera_position", camera_position, NULL, camera_state_flag);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Camera Direction");
        ImGui::SameLine();
        ImGui::InputFloat3("##camera_direction", camera_direction, NULL, camera_state_flag);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Direction Index");
        ImGui::SameLine();
        ImGui::InputInt("##direction_index", &direction_index, 1, 100, camera_state_flag);

        this->gui_data.ihm_index = ihm_index;
        this->gui_data.camera_position.x = camera_position[0];
        this->gui_data.camera_position.y = camera_position[1];
        this->gui_data.camera_position.z = camera_position[2];
        this->gui_data.camera_rotation_index = direction_index;
    }

    if (ImGui::CollapsingHeader("World Building")) {
        this->gui_data.is_stochastic_subtraction = ImGui::Button("Stochastic Subtraction");
    }

    if (ImGui::CollapsingHeader("IHM Testing")) {
        this->gui_data.is_verify_ihm = ImGui::Button("Verify");
        this->gui_data.is_estimate_position = ImGui::Button("Estimate Position");
        this->gui_data.is_save_heatmap = ImGui::Button("Save Heatmap");
        this->gui_data.is_save_confusion = ImGui::Button("Save Confusion Map");
    }

    ImGui::End();
}

void MainGui::ToggleButton(const char* str_id, const char* label, bool* v)
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;
    float rounding = 0.2f;

    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked()) *v = !*v;
    ImGuiContext& gg = *GImGui;
    float ANIM_SPEED = 0.085f;
    if (gg.LastActiveId == gg.CurrentWindow->GetID(str_id))
        float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);
    if (ImGui::IsItemHovered())
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * rounding);
    else
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * rounding);

    ImVec2 center = ImVec2(radius + (*v ? 1 : 0) * (width - radius * 1.85f), radius);
    draw_list->AddRectFilled(ImVec2((p.x + center.x) - 9.0f, p.y + 1.5f),
        ImVec2((p.x + (width / 2) + center.x) - 9.0f, p.y + height - 1.5f), IM_COL32(255, 255, 255, 255), height * rounding);
    
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(label);
}

void MainGui::DrawCameraSelector() {
    bool item_highlight = false;
    int item_highlighted_idx = -1;

    ImGui::SeparatorText("Camera Select");
    if (ImGui::BeginListBox("##camera_select"))
    {
        for (int i = 0; i < this->camera_count; i++)
        {
            const bool is_selected = (this->camera_index == i);
            if (ImGui::Selectable(this->camera_labels[i].c_str(), is_selected))
                this->camera_index = i;

            if (item_highlight && ImGui::IsItemHovered())
                item_highlighted_idx = i;

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
    ImGui::Separator();
}

void MainGui::Cleanup() {
    printf("Cleaning Vulkan...\n");
    this->vulkan_pipeline->Cleanup();
    printf("    Done!\n\n");
}

bool MainGui::IsWindowClosed() {
    return this->vulkan_pipeline->vulkan_renderer->IsWindowClosed();
}
