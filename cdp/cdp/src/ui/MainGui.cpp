
#include "MainGui.h"

MainGui::MainGui() {
    this->vulkan_pipeline = new VulkanPipeline();
}

void MainGui::Update() {
    bool show_demo_window = true;

    bool is_renderable = this->vulkan_pipeline->Update();

    if (!is_renderable) {
        return;
    }

    //ImGui::ShowDemoWindow(&show_demo_window);
    this->DrawViewport();
    this->DrawInspector();

    this->vulkan_pipeline->Render(this->desktop_color);
}

void MainGui::DrawViewport() {
    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Once);

    ImGui::Begin("Render Viewport");

    if (ImGui::CollapsingHeader("Metadata")) {
        ImGui::Text("[ms/F]: %.2f", 1000.0f / this->vulkan_pipeline->io->Framerate);
        ImGui::Text("[FP/s]: %.1f", this->vulkan_pipeline->io->Framerate);
        ImGui::Text("[X, Y]: %.1f %.1f", this->vulkan_pipeline->io->MousePos[0], this->vulkan_pipeline->io->MousePos[1]);
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

    ImGui::End();
}

void MainGui::DrawInspector() {
    ImGui::SetNextWindowSize(ImVec2(300, 660), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(940, 30), ImGuiCond_Once);

    ImGui::Begin("Inspector");

    if (ImGui::CollapsingHeader("Metadata")) {
        ImGui::Text("[ms/F]: %.2f", 1000.0f / this->vulkan_pipeline->io->Framerate);
        ImGui::Text("[FP/s]: %.1f", this->vulkan_pipeline->io->Framerate);
        ImGui::Text("[X, Y]: %.1f %.1f", this->vulkan_pipeline->io->MousePos[0], this->vulkan_pipeline->io->MousePos[1]);
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
    this->vulkan_pipeline->Cleanup();
}

bool MainGui::IsWindowClosed() {
    return this->vulkan_pipeline->IsWindowClosed();
}
