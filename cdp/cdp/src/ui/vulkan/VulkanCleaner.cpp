#include "VulkanCleaner.h"

VulkanCleaner::VulkanCleaner(VulkanCore* vulkan_core) {
    this->vulkan_core = vulkan_core;
}

void VulkanCleaner::Cleanup(std::vector<VulkanTexture*> texture_list) {
    printf("    Waiting for device to finish...\n");
    this->vulkan_core->err = vkDeviceWaitIdle(this->vulkan_core->g_Device);
    this->vulkan_core->check_vk_result(this->vulkan_core->err);
    printf("    ImGUI-Vulkan Shutdown...\n");
    ImGui_ImplVulkan_Shutdown();
    printf("    ImGUI-GLFW Shutdown...\n");
    ImGui_ImplGlfw_Shutdown();
    printf("    Destroying ImGUI Context...\n");
    ImGui::DestroyContext();

    printf("    Cleaning Vulkan Window...\n");
    this->CleanupVulkanWindow();
    printf("    Cleaning Vulkan Instance...\n");
    this->CleanupVulkan(texture_list);

    printf("    GLFW terminate...\n");
    glfwDestroyWindow(this->vulkan_core->window);
    glfwTerminate();
}


void VulkanCleaner::CleanupVulkan(std::vector<VulkanTexture*> texture_list)
{
    vkDestroyDescriptorPool(this->vulkan_core->g_Device, this->vulkan_core->g_DescriptorPool, this->vulkan_core->g_Allocator);
    
    for (int i = 0; i < texture_list.size(); i++) {
        texture_list[i]->RemoveTexture();
    }

    vkDestroyDevice(this->vulkan_core->g_Device, this->vulkan_core->g_Allocator);
    vkDestroyInstance(this->vulkan_core->g_Instance, this->vulkan_core->g_Allocator);
}

void VulkanCleaner::CleanupVulkanWindow()
{
    ImGui_ImplVulkanH_DestroyWindow(this->vulkan_core->g_Instance, this->vulkan_core->g_Device, &this->vulkan_core->g_MainWindowData, this->vulkan_core->g_Allocator);
}