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

		std::vector<VulkanTexture*> texture_list;

		VulkanPipeline();
		bool Update();
		void Render();
		void Cleanup();

		/*
		bool Update();
		void Render(ImVec4 clear_color);
		void Cleanup();
		bool IsWindowClosed();

		void SetupVulkan(ImVector<const char*> extensions);
		void CreateInstance(ImVector<const char*> instance_extensions);
		void SetupDevice();
		void CreateDescriptorPool();
		VkPhysicalDevice SetupVulkan_SelectPhysicalDevice();
		static void glfw_error_callback(int error, const char* description);
		static void check_vk_result(VkResult err);
		static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension);
		void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int w, int h);
		void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
		void FramePresent(ImGui_ImplVulkanH_Window* wd);
		void CleanupVulkan();
		void CleanupVulkanWindow();
		bool CheckValidationLayerSupport();
		*/
};