#pragma once

#include <iostream>
#include <string>
#include <format>
#include <vector>

#include "imgui_internal.h"
#include "imfilebrowser.h"

#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanTexture.h"

#include "../engine/Math.h"
#include "../engine/Indexer.h"
#include "../entity/ai/phm/PhmGenerator.h"
#include "GuiData.h"

struct MainGui {
	int render_texture = 0;
	PhmGenerator phm_generator{};

	int camera_index = 0;
	int camera_count = 0;

	std::vector<std::string> camera_labels{};

	GuiData* gui_data = new GuiData{};

	ImGui::FileBrowser save_proximity_dialog = ImGui::FileBrowser(ImGuiFileBrowserFlags_EnterNewFilename);
	ImGui::FileBrowser load_env_dialog;

	float uv_offset = 0.0f;
	float uv_delta = 0.00005f;

	VulkanPipeline* vulkan_pipeline;

    void Init(int camera_count) {
        this->save_proximity_dialog.SetTitle("Proximity Hash Save Location");
        this->load_env_dialog.SetTitle("Load Environment");
        this->save_proximity_dialog.SetTypeFilters({ ".proximity" });
        this->load_env_dialog.SetTypeFilters({ ".ply" });

        this->camera_count = camera_count;

        for (int i = 0; i < this->camera_count; i++) {
            std::string camera_label = "Camera " + std::to_string(i);
            this->camera_labels.push_back(camera_label);
        }

        this->vulkan_pipeline = new VulkanPipeline();
        this->vulkan_pipeline->Init(this->camera_count);

        this->phm_generator.Init(
            3,
            Vector2{ -Math::Pi() / 4.0f, 0 },
            Vector::ZERO3(),
            Vector3{ 1000, 100, 1000 },
            Vector3{ 1000, 100, 1000 },
            Vector3{ 10, 10, 10 },
            Vector2{ 16, 16 }
        );
        this->gui_data->ihm_index = Indexer::FlatIndex4(
            (float)this->gui_data->camera_rotation_index,
            this->gui_data->camera_position.x,
            this->gui_data->camera_position.y,
            this->gui_data->camera_position.z,
            (float)this->phm_generator.direction_count,
            this->phm_generator.world_width_strided.x,
            this->phm_generator.world_width_strided.y
        );
    }

    void Update() {
        bool is_renderable = this->vulkan_pipeline->Update();

        if (!is_renderable) {
            return;
        }

        this->DrawBackground();
        this->DrawViewport();
        this->DrawInspector();
        //bool show_demo_window = true;
        //ImGui::ShowDemoWindow(&show_demo_window);

        this->save_proximity_dialog.Display();
        this->load_env_dialog.Display();

        this->RefreshGuiData();

        this->vulkan_pipeline->Render();
    }

    void RefreshGuiData() {
        this->gui_data->is_window_open = !this->IsWindowClosed();

        this->gui_data->camera_index = this->camera_index;

        if (this->save_proximity_dialog.HasSelected()) {
            this->gui_data->save_phash_path = this->save_proximity_dialog.GetSelected().string();
            this->save_proximity_dialog.ClearSelected();
        }
        else {
            this->gui_data->save_phash_path = "";
        }

        if (this->load_env_dialog.HasSelected()) {
            this->gui_data->load_env_path = this->load_env_dialog.GetSelected().string();
            this->load_env_dialog.ClearSelected();
        }
        else {
            this->gui_data->load_env_path = "";
        }
    }

    void DrawBackground() {
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

    void DrawViewport() {
        ImGuiWindowFlags window_settings = ImGuiWindowFlags_MenuBar;
        bool is_open;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Once);

        ImGui::Begin("Render Viewport", &is_open, ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Load Environment")) {
                    this->load_env_dialog.Open();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Save Proximity Hash")) {
                    this->save_proximity_dialog.Open();
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }


        ImVec2 img_size = ImGui::GetContentRegionAvail();
        ImTextureID image_texture = (ImTextureID)this->vulkan_pipeline->texture_list[1]->instance_descriptor;

        if (this->gui_data->is_simulating and this->camera_count > 0) {
            if (this->render_texture == 0) {
                image_texture = (ImTextureID)this->vulkan_pipeline->depth_textures[this->camera_index]->instance_descriptor;
            }
            else if (this->render_texture == 1) {
                image_texture = (ImTextureID)this->vulkan_pipeline->phash_textures[this->camera_index]->instance_descriptor;
            }
            else if (this->render_texture == 2) {
                image_texture = (ImTextureID)this->vulkan_pipeline->height_textures[this->camera_index]->instance_descriptor;
            }
        }

        ImGui::Image(image_texture, img_size);

        ImGui::End();
        ImGui::PopStyleVar(1);
    }

    void DrawInspector() {
        ImGui::SetNextWindowSize(ImVec2(340, 660), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(900, 30), ImGuiCond_Once);

        ImGui::Begin("Inspector");

        if (!ImGui::CollapsingHeader("Metadata")) {
            ImGui::Text("[ms/F]: %.2f", 1000.0f / this->vulkan_pipeline->vulkan_core->io->Framerate);
            ImGui::Text("[FP/s]: %.1f", this->vulkan_pipeline->vulkan_core->io->Framerate);
            ImGui::Text("[X, Y]: %.1f %.1f", this->vulkan_pipeline->vulkan_core->io->MousePos[0], this->vulkan_pipeline->vulkan_core->io->MousePos[1]);
        }

        if (!ImGui::CollapsingHeader("Simulation")) {
            this->ToggleButton("is_simulating", "Run Simulation", &this->gui_data->is_simulating);
            ImGui::SameLine();
            this->gui_data->is_save_simulation_image = ImGui::Button("Save Sim Image");
            this->ToggleButton("is_paused", "Pause ||", &this->gui_data->is_paused);
            this->gui_data->is_step_simulation = ImGui::Button("  Step ||>  ");

            this->DrawCameraSelector();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Render Texture");
            ImGui::SameLine();
            ImGui::Combo("##", &this->render_texture, "Depth\0P-Hash\0Height Hash\0\0");
        }

        if (!ImGui::CollapsingHeader("Camera")) {
            this->gui_data->keyboard = Vector::ZERO3();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Control Method");
            ImGui::SameLine();
            const char* selector_options[] = { "None", "IHM Index", "Camera State", "IHM Keyboard", "Drone Keyboard", "Wallrider" };
            ImGui::Combo("##control_selector", &this->gui_data->control_index, selector_options, IM_ARRAYSIZE(selector_options));

            ImGui::Separator();

            ImGui::Text("Noise");
            ImGui::SameLine();
            ImGui::Checkbox("##is_rotation_noise", &this->gui_data->is_rotation_noise);

            ImGui::Separator();

            unsigned long long ihm_index = this->gui_data->ihm_index;

            float camera_position[3] = { this->gui_data->camera_position.x, this->gui_data->camera_position.y, this->gui_data->camera_position.z };
            int direction_index = this->gui_data->camera_rotation_index;
            ImGuiInputTextFlags_ ihm_index_flag = ImGuiInputTextFlags_None;
            ImGuiInputTextFlags_ camera_state_flag = ImGuiInputTextFlags_None;

            if (this->gui_data->control_index == 0) {
                ihm_index_flag = ImGuiInputTextFlags_ReadOnly;
                camera_state_flag = ImGuiInputTextFlags_ReadOnly;
            }
            else if (this->gui_data->control_index == 1) {
                camera_state_flag = ImGuiInputTextFlags_ReadOnly;
                PhmState phm_state = this->phm_generator.GetPhmState(ihm_index, false);

                camera_position[0] = phm_state.position_strided.x;
                camera_position[1] = phm_state.position_strided.y;
                camera_position[2] = phm_state.position_strided.z;
                direction_index = phm_state.direction_index;
            }
            else if (this->gui_data->control_index == 2) {
                ihm_index_flag = ImGuiInputTextFlags_ReadOnly;
                ihm_index = Indexer::FlatIndex4((float)direction_index, camera_position[0], camera_position[1], camera_position[2], (float)this->phm_generator.direction_count, this->phm_generator.world_width_strided.x, this->phm_generator.world_width_strided.y);
            }
            else if (this->gui_data->control_index == 3) {
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

                camera_position[0] = (float)Math::Clip(camera_position[0], this->phm_generator.world_origin.x, this->phm_generator.world_width_strided.x - 1);
                camera_position[1] = (float)Math::Clip(camera_position[1], this->phm_generator.world_origin.y, this->phm_generator.world_width_strided.y - 1);
                camera_position[2] = (float)Math::Clip(camera_position[2], this->phm_generator.world_origin.z, this->phm_generator.world_width_strided.z - 1);
                direction_index = (int)Math::Clip(direction_index, 0, this->phm_generator.direction_count - 1);

                ihm_index = Indexer::FlatIndex4((float)direction_index, camera_position[0], camera_position[1], camera_position[2], (float)this->phm_generator.direction_count, this->phm_generator.world_width_strided.x, this->phm_generator.world_width_strided.y);
            }
            else if (this->gui_data->control_index == 4) {
                ihm_index_flag = ImGuiInputTextFlags_ReadOnly;
                camera_state_flag = ImGuiInputTextFlags_ReadOnly;

                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_W))) {
                    this->gui_data->keyboard.z = 1;
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_S))) {
                    this->gui_data->keyboard.z = -1;
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A))) {
                    this->gui_data->keyboard.x = -1;
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_D))) {
                    this->gui_data->keyboard.x = 1;
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Q))) {
                    this->gui_data->keyboard.y = -1;
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_E))) {
                    this->gui_data->keyboard.y = 1;
                }
            }

            Vector3 direction = this->phm_generator.directions_cpu[direction_index];
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

            this->gui_data->ihm_index = ihm_index;
            this->gui_data->camera_position.x = camera_position[0];
            this->gui_data->camera_position.y = camera_position[1];
            this->gui_data->camera_position.z = camera_position[2];
            this->gui_data->camera_rotation_index = direction_index;
        }

        if (!ImGui::CollapsingHeader("Drone Alpha")) {
            float drone_voxel[3] = { this->gui_data->drone_voxel.x, this->gui_data->drone_voxel.y, this->gui_data->drone_voxel.z };
            float drone_position[3] = { this->gui_data->drone_position.x, this->gui_data->drone_position.y, this->gui_data->drone_position.z };
            float drone_forward[3] = { this->gui_data->drone_forward.x, this->gui_data->drone_forward.y, this->gui_data->drone_forward.z };
            float drone_quaternion[4] = { this->gui_data->drone_quaternion.x, this->gui_data->drone_quaternion.y, this->gui_data->drone_quaternion.z, this->gui_data->drone_quaternion.w };
            float drone_speed[1] = { Transform::Norm3(this->gui_data->drone_velocity) };
            float drone_velocity[3] = { this->gui_data->drone_velocity.x, this->gui_data->drone_velocity.y, this->gui_data->drone_velocity.z };
            float drone_angular_velocity[3] = { this->gui_data->drone_angular_velocity.x, this->gui_data->drone_angular_velocity.y, this->gui_data->drone_angular_velocity.z };
            float wallride_sensor[3] = { this->gui_data->wallride_sensor.x, this->gui_data->wallride_sensor.y, this->gui_data->wallride_sensor.z };

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Voxel");
            ImGui::SameLine();
            ImGui::InputFloat3("##drone_voxel", drone_voxel, NULL, ImGuiInputTextFlags_ReadOnly);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Position");
            ImGui::SameLine();
            ImGui::InputFloat3("##drone_position", drone_position, NULL, ImGuiInputTextFlags_ReadOnly);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Forward");
            ImGui::SameLine();
            ImGui::InputFloat3("##drone_forward", drone_forward, NULL, ImGuiInputTextFlags_ReadOnly);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Quaternion");
            ImGui::SameLine();
            ImGui::InputFloat4("##drone_quaternion", drone_quaternion, NULL, ImGuiInputTextFlags_ReadOnly);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Velocity");
            ImGui::SameLine();
            ImGui::InputFloat3("##drone_velocity", drone_velocity, NULL, ImGuiInputTextFlags_ReadOnly);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Speed");
            ImGui::SameLine();
            ImGui::InputFloat("##drone_speed", drone_speed, NULL, ImGuiInputTextFlags_ReadOnly);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Angular Velocity");
            ImGui::SameLine();
            ImGui::InputFloat3("##drone_angular_velocity", drone_angular_velocity, NULL, ImGuiInputTextFlags_ReadOnly);

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Wallride Forward");
            ImGui::SameLine();
            ImGui::InputFloat3("##wallride_forward", wallride_sensor, NULL, ImGuiInputTextFlags_ReadOnly);
        }

        if (ImGui::CollapsingHeader("World Building")) {
            this->gui_data->is_stochastic_subtraction = ImGui::Button("Stochastic Subtraction");
        }

        if (ImGui::CollapsingHeader("Training Data")) {
            this->gui_data->is_generate_hitpoly_data = ImGui::Button("Hitpoly Data");
            this->gui_data->is_generate_phm_data = ImGui::Button("Perceptual Hash Matrix");
            this->gui_data->is_generate_shm_data = ImGui::Button("Similarity Hash Matrix");
        }

        if (ImGui::CollapsingHeader("Testing")) {
            this->gui_data->is_playground = ImGui::Button("Playground");
        }

        ImGui::End();
    }

    void ToggleButton(const char* str_id, const char* label, bool* v)
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

    void DrawCameraSelector() {
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

    void Cleanup() {
        printf("Cleaning Vulkan...\n");
        this->vulkan_pipeline->Cleanup();
        printf("    Done!\n\n");
    }

    bool IsWindowClosed() {
        return this->vulkan_pipeline->vulkan_renderer->IsWindowClosed();
    }

};