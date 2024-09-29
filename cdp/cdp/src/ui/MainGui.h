#pragma once

#include "VulkanPipeline.h"

#include <iostream>

class MainGui {
	public:
		bool is_checked0 = false;
		bool is_checked1 = false;
		bool is_checked2 = false;
		float slider0 = 0;
		float slider1 = 0.5f;
		float slider2 = 1.0f;

		ImVec4 desktop_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImVec4 color0 = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		ImVec4 color1 = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);
		ImVec4 color2 = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

		VulkanPipeline* vulkan_pipeline;

		MainGui();
		void Update();
		void Cleanup();
		bool IsWindowClosed();
};