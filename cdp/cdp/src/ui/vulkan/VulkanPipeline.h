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

class VulkanPipeline {
	public:

		VulkanCore* vulkan_core;
		VulkanRenderer* vulkan_renderer;
		VulkanCleaner* vulkan_cleaner;

		std::vector<VulkanTexture*> depth_textures;
		std::vector<VulkanTexture*> phash_textures;
		std::vector<VulkanTexture*> shaded_textures;
		std::vector<VulkanTexture*> texture_list;

		VulkanPipeline(int camera_count);
		bool Update();
		void Render();
		void Cleanup();
		std::vector<CUdeviceptr> GetDepthTextures();
		std::vector<CUdeviceptr> GetPhashTextures();
		std::vector<CUdeviceptr> GetShadedTextures();
};