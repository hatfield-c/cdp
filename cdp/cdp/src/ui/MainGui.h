#pragma once

#include "VulkanPipeline.h"

#include <iostream>

class MainGui {
	public:
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		VulkanPipeline vulkan_pipeline = VulkanPipeline();

		MainGui();
		void Loop();

};