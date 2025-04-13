#pragma once

// Dear ImGui: standalone example application for Glfw + Vulkan
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

#include "VulkanCore.h"
#include "VulkanRenderer.h"
#include "VulkanCleaner.h"
#include "VulkanTexture.h"

struct VulkanPipeline {
	VulkanCore* vulkan_core;
	VulkanRenderer* vulkan_renderer;
	VulkanCleaner* vulkan_cleaner;

	std::vector<VulkanTexture*> depth_textures;
	std::vector<VulkanTexture*> phash_textures;
	std::vector<VulkanTexture*> height_textures;
	std::vector<VulkanTexture*> texture_list;

    void Init(int camera_count) {
        int ui_texture_count = 3;
        int descriptor_count = (3 * camera_count) + ui_texture_count;

        this->vulkan_core = new VulkanCore();
        this->vulkan_renderer = new VulkanRenderer();
        this->vulkan_cleaner = new VulkanCleaner();

        this->vulkan_core->Init(descriptor_count);
        this->vulkan_renderer->Init(this->vulkan_core);
        this->vulkan_cleaner->Init(this->vulkan_core);

        VulkanTexture* desktop_texture = new VulkanTexture();
        desktop_texture->Init(this->vulkan_core);
        bool result = desktop_texture->LoadImage("data/media/desktop.jpg");

        this->texture_list.push_back(desktop_texture);

        VulkanTexture* viewport_texture = new VulkanTexture();
        viewport_texture->Init(this->vulkan_core);
        result = viewport_texture->LoadImage("data/media/viewport_default.jpg");

        this->texture_list.push_back(viewport_texture);

        for (int i = 0; i < camera_count; i++) {
            VulkanTexture* depth_texture = new VulkanTexture();
            VulkanTexture* phash_texture = new VulkanTexture();
            VulkanTexture* centroid_texture = new VulkanTexture();

            depth_texture->Init(this->vulkan_core);
            phash_texture->Init(this->vulkan_core);
            centroid_texture->Init(this->vulkan_core);

            result = depth_texture->LoadImage("data/media/viewport_default.jpg");
            result = phash_texture->LoadImage("data/media/phash_default.jpg");
            result = centroid_texture->LoadImage("data/media/phash_default.jpg");

            this->depth_textures.push_back(depth_texture);
            this->phash_textures.push_back(phash_texture);
            this->height_textures.push_back(centroid_texture);

            this->texture_list.push_back(depth_texture);
            this->texture_list.push_back(phash_texture);
            this->texture_list.push_back(centroid_texture);
        }
    }

    bool Update() {
        return this->vulkan_renderer->Update();
    }

    void Render() {
        this->vulkan_renderer->Render();
    }

    void Cleanup() {
        this->vulkan_cleaner->Cleanup(this->texture_list);
    }

    std::vector<CUdeviceptr> GetDepthTextures() {
        std::vector<CUdeviceptr> textures{};

        for (int i = 0; i < this->depth_textures.size(); i++) {
            CUdeviceptr gpu_texture = this->depth_textures[i]->ExportAsCuda();

            textures.push_back(gpu_texture);
        }

        return textures;
    }

    std::vector<CUdeviceptr> GetPhashTextures() {
        std::vector<CUdeviceptr> textures{};

        for (int i = 0; i < this->depth_textures.size(); i++) {
            CUdeviceptr gpu_texture = this->phash_textures[i]->ExportAsCuda();

            textures.push_back(gpu_texture);
        }

        return textures;
    }

    std::vector<CUdeviceptr> GetHeightTextures() {
        std::vector<CUdeviceptr> textures{};

        for (int i = 0; i < this->height_textures.size(); i++) {
            CUdeviceptr gpu_texture = this->height_textures[i]->ExportAsCuda();

            textures.push_back(gpu_texture);
        }

        return textures;
    }

};