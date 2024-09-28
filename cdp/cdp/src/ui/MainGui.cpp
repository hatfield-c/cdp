
#include "MainGui.h"

MainGui::MainGui() {
    
}

void MainGui::Update() {
    bool show_demo_window = true;

    bool is_renderable = this->vulkan_pipeline.Update();

    if (!is_renderable) {
        return;
    }
    ImGui::ShowDemoWindow(&show_demo_window);
    ImGui::Begin("Inspector");

    if (ImGui::CollapsingHeader("Metadata")) {
        ImGui::Text("%.2f ms/frame %.1f FPS", 1000.0f / this->vulkan_pipeline.io.Framerate);
        ImGui::Text("%.1f FPS", this->vulkan_pipeline.io.Framerate);
    }
    /*
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
    */
    ImGui::End();

    //ImGui::ShowDemoWindow(&show_demo_window);
    /*
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float*)&this->clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / this->vulkan_pipeline.io.Framerate, this->vulkan_pipeline.io.Framerate);
    ImGui::End();
    */
    this->vulkan_pipeline.Render(this->desktop_color);
}

void MainGui::Cleanup() {
    this->vulkan_pipeline.Cleanup();
}

bool MainGui::IsWindowClosed() {
    return this->vulkan_pipeline.IsWindowClosed();
}
