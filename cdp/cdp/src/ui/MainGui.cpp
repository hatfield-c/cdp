
#include "MainGui.h"

MainGui::MainGui(int camera_count) {
    this->load_env_dialog.SetTitle("Load Environment");
    this->save_env_dialog.SetTitle("Save Environment");
    this->load_env_dialog.SetTypeFilters({ ".ply" });
    this->save_env_dialog.SetTypeFilters({ ".ply" });

    this->camera_count = camera_count;

    for (int i = 0; i < this->camera_count; i++) {
        std::string camera_label = "Camera " + std::to_string(i);
        this->camera_labels.push_back(camera_label);
    }

    this->vulkan_pipeline = new VulkanPipeline(this->camera_count);
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

    this->RefreshGuiData();

    this->vulkan_pipeline->Render();
}

void MainGui::RefreshGuiData() {
    this->gui_data.is_window_open = !this->IsWindowClosed();

    Vector3 gui_position_data{
        this->render_position[0],
        this->render_position[1],
        this->render_position[2]
    };
    Vector3 gui_rotation_data{
        this->render_rotation[0],
        this->render_rotation[1],
        this->render_rotation[2]
    };
    unsigned long long gui_index_data = this->ihm_position_index;

    bool is_position_changed = gui_position_data != this->gui_data.render_position;
    bool is_rotation_changed = gui_rotation_data != this->gui_data.render_rotation;
    bool is_index_changed = gui_index_data != this->gui_data.ihm_position_index;

    if (is_position_changed || is_rotation_changed) {

    }

    if (is_index_changed) {

    }

    this->gui_data.render_position = gui_position_data;
    this->gui_data.render_rotation = gui_rotation_data;
    this->gui_data.ihm_position_index = gui_index_data;
    this->gui_data.camera_index = this->camera_index;

    if (this->load_env_dialog.HasSelected()) {
        this->gui_data.load_path = this->load_env_dialog.GetSelected().string();
        this->load_env_dialog.ClearSelected();
    } else {
        this->gui_data.load_path = "";
    }

    if (this->save_env_dialog.HasSelected()) {
        this->gui_data.save_path = this->save_env_dialog.GetSelected().string();
        this->save_env_dialog.ClearSelected();
    } else {
        this->gui_data.save_path = "";
    }

    this->gui_data.vote_threshold = this->vote_threshold;
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

    if (!ImGui::CollapsingHeader("p-Hash")) {
        int min_val = 0;
        int max_val = 8;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Vote Threshold");
        ImGui::SameLine();
        ImGui::SliderScalar("##phash_votes", ImGuiDataType_U8, &this->vote_threshold, &min_val, &max_val);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Position");
        ImGui::SameLine();
        ImGui::InputInt3("##ihm_position", this->render_position);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Rotation");
        ImGui::SameLine();
        ImGui::InputFloat3("##ihm_rotation", this->render_rotation);

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("IHM Index");
        ImGui::SameLine();
        //ImGui::InputFloat3("##ihm_index", this->ihm_position_index);
        unsigned long long one_val = 1;
        ImGui::InputScalar("##ihm_index", ImGuiDataType_U64, &this->ihm_position_index, &one_val, NULL, NULL, ImGuiInputTextFlags_None);
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
