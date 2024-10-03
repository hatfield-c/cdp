#pragma once

#include "VulkanCore.h"

class VulkanRenderer {
	public:
		ImVec4 default_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		VulkanCore* vulkan_core;

		VulkanRenderer(VulkanCore* vulkan_core);
		bool Update();
		bool IsWindowClosed();
		void Render();
		void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
		void FramePresent(ImGui_ImplVulkanH_Window* wd);
};