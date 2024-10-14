
#include "VulkanPipeline.h"

VulkanPipeline::VulkanPipeline() {
    this->vulkan_core = new VulkanCore();
    this->vulkan_renderer = new VulkanRenderer(this->vulkan_core);
    this->vulkan_cleaner = new VulkanCleaner(this->vulkan_core);

    VulkanTexture* desktop_texture = new VulkanTexture(this->vulkan_core);
    bool result = desktop_texture->LoadImage("data/media/desktop.jpg");
    
    this->texture_list.push_back(desktop_texture);

    VulkanTexture* viewport_texture = new VulkanTexture(this->vulkan_core);
    result = viewport_texture->LoadImage("data/media/viewport_default.jpg");

    this->texture_list.push_back(viewport_texture);
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

CUdeviceptr VulkanPipeline::GetViewportImage() {
    CUdeviceptr viewport_image = this->texture_list[1]->ExportAsCuda();

    return viewport_image;
}
