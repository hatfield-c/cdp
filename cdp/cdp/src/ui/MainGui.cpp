
#include "MainGui.h"

MainGui::MainGui(int camera_count) {
    this->camera_count = camera_count;

    for (int i = 0; i < this->camera_count; i++) {
        std::string camera_label = "Camera " + std::to_string(i);
        this->camera_labels.push_back(camera_label);
    }

    this->vulkan_pipeline = new VulkanPipeline(this->camera_count);
}

void MainGui::Update() {
    
    bool show_demo_window = true;
    
    bool is_renderable = this->vulkan_pipeline->Update();

    if (!is_renderable) {
        return;
    }
    
    this->DrawBackground();
    this->DrawViewport();
    this->DrawInspector();
    //ImGui::ShowDemoWindow(&show_demo_window);
    
    this->vulkan_pipeline->Render();
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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Once);

    ImGui::Begin("Render Viewport");

    ImVec2 img_size = ImGui::GetContentRegionAvail();
    
    if(this->is_simulating and this->camera_count > 0) {
        ImGui::Image((ImTextureID)this->vulkan_pipeline->camera_textures[this->camera_index]->instance_descriptor, img_size);
    }
    else {
        ImGui::Image((ImTextureID)this->vulkan_pipeline->texture_list[1]->instance_descriptor, img_size);
    }
    
    ImGui::End();
    ImGui::PopStyleVar(1);
}

void MainGui::DrawInspector() {
    ImGui::SetNextWindowSize(ImVec2(300, 660), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(940, 30), ImGuiCond_Once);

    ImGui::Begin("Inspector");

    this->ToggleButton("is_simulating", "Run Simulation", &this->is_simulating);
    this->DrawCameraSelector();

    if (ImGui::CollapsingHeader("Metadata")) {
        ImGui::Text("[ms/F]: %.2f", 1000.0f / this->vulkan_pipeline->vulkan_core->io->Framerate);
        ImGui::Text("[FP/s]: %.1f", this->vulkan_pipeline->vulkan_core->io->Framerate);
        ImGui::Text("[X, Y]: %.1f %.1f", this->vulkan_pipeline->vulkan_core->io->MousePos[0], this->vulkan_pipeline->vulkan_core->io->MousePos[1]);
    }

    if (ImGui::CollapsingHeader("Text")) {
        ImGui::Text("This is some useful text.");
        ImGui::Text("This is some useful text.");
        ImGui::Text("This is some useful text.");
    }

    if (ImGui::CollapsingHeader("Checkbox")) {
        ImGui::Checkbox("Checkbox0", &this->is_checked0);
        ImGui::Checkbox("Checkbox1", &this->is_checked1);
        ImGui::Checkbox("Checkbox2", &this->is_checked2);
    }

    if (ImGui::CollapsingHeader("Slider")) {
        ImGui::SliderFloat("Slider0", &this->slider0, 0.0f, 1.0f);
        ImGui::SliderFloat("Slider1", &this->slider1, 0.0f, 1.0f);
        ImGui::SliderFloat("Slider2", &this->slider2, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Colors")) {
        ImGui::ColorEdit3("desktop", (float*)&this->desktop_color);
        ImGui::ColorEdit4("Color0", (float*)&this->color0);
        ImGui::ColorEdit4("Color1", (float*)&this->color1);
        ImGui::ColorEdit4("Color2", (float*)&this->color2);
    }

    if (ImGui::CollapsingHeader("Buttons")) {
        int empty = 0;
        if (ImGui::Button("Button0")) {
            empty = 1;
        }

        if (ImGui::Button("Button1")) {
            empty = 2;
        }

        if (ImGui::Button("Button2")) {
            empty = 3;
        }
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
    printf("    Done!\n");
}

bool MainGui::IsWindowClosed() {
    return this->vulkan_pipeline->vulkan_renderer->IsWindowClosed();
}