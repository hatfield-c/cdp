
#include "VulkanPipeline.h"

VulkanPipeline::VulkanPipeline() {
    this->vulkan_core = new VulkanCore();
    this->vulkan_renderer = new VulkanRenderer(this->vulkan_core);
    this->vulkan_cleaner = new VulkanCleaner(this->vulkan_core);

    VulkanTexture* vulkan_texture = new VulkanTexture(this->vulkan_core);
    bool result = vulkan_texture->LoadImage("data/media/desktop.jpg");
    
    this->texture_list.push_back(vulkan_texture);

}

bool VulkanPipeline::Update() {
    return this->vulkan_renderer->Update();
}

void VulkanPipeline::Render() {
    this->vulkan_renderer->Render();
}

void VulkanPipeline::Cleanup() {
    this->vulkan_cleaner->Cleanup(this->texture_list);
}
