#pragma once

#include "VulkanCore.h"
#include "VulkanTexture.h"

class VulkanCleaner {
public:
	VulkanCore* vulkan_core;

	VulkanCleaner(VulkanCore* vulkan_core);
    void Cleanup(std::vector<VulkanTexture*> texture_list);
    void CleanupVulkan(std::vector<VulkanTexture*> texture_list);
    void CleanupVulkanWindow();
};