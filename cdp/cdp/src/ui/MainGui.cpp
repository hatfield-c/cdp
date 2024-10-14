
#include "MainGui.h"

MainGui::MainGui() {
    this->vulkan_pipeline = new VulkanPipeline();

    CUdeviceptr viewport_image = this->vulkan_pipeline->GetViewportImage();
    this->viewport_renderer = new ViewportRenderer(viewport_image);
}

void MainGui::Update() {
    bool show_demo_window = true;
    
    bool is_renderable = this->vulkan_pipeline->Update();

    if (!is_renderable) {
        return;
    }
    
    //ImGui::ShowDemoWindow(&show_demo_window);
    this->DrawBackground();
    this->DrawViewport();
    this->DrawInspector();

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
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Once);

    ImGui::Begin("Render Viewport");

    ImVec2 img_size = ImGui::GetContentRegionAvail();
    ImGui::Image((ImTextureID)this->vulkan_pipeline->texture_list[1]->instance_descriptor, img_size);

    ImGui::End();
    ImGui::PopStyleVar(1);
}

void MainGui::DrawInspector() {
    ImGui::SetNextWindowSize(ImVec2(300, 660), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(940, 30), ImGuiCond_Once);

    ImGui::Begin("Inspector");

    if (ImGui::Button("Start Viewport")) {
        this->viewport_renderer->Render();
    }

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

void MainGui::Cleanup() {
    printf("Cleaning Cuda...\n");
    this->viewport_renderer->Cleanup();
    printf("    Done!\n");

    printf("Cleaning Vulkan...\n");
    this->vulkan_pipeline->Cleanup();
    printf("    Done!\n");
}

bool MainGui::IsWindowClosed() {
    return this->vulkan_pipeline->vulkan_renderer->IsWindowClosed();
}