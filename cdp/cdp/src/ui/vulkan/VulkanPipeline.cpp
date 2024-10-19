
#include "VulkanPipeline.h"

VulkanPipeline::VulkanPipeline(int camera_count) {
    int descriptor_count = camera_count + 32;

    this->vulkan_core = new VulkanCore(descriptor_count);
    this->vulkan_renderer = new VulkanRenderer(this->vulkan_core);
    this->vulkan_cleaner = new VulkanCleaner(this->vulkan_core);

    VulkanTexture* desktop_texture = new VulkanTexture(this->vulkan_core);
    bool result = desktop_texture->LoadImage("data/media/desktop.jpg");
    
    this->texture_list.push_back(desktop_texture);

    VulkanTexture* viewport_texture = new VulkanTexture(this->vulkan_core);
    result = viewport_texture->LoadImage("data/media/viewport_default.jpg");

    this->texture_list.push_back(viewport_texture);

    for (int i = 0; i < camera_count; i++) {
        VulkanTexture* texture = new VulkanTexture(this->vulkan_core);
        result = texture->LoadImage("data/media/viewport_default.jpg");

        this->camera_textures.push_back(texture);
        this->texture_list.push_back(texture);
    }
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

std::vector<CUdeviceptr> VulkanPipeline::GetCameraTextures() {
    std::vector<CUdeviceptr> camera_textures{};

    for (int i = 0; i < this->camera_textures.size(); i++) {
        CUdeviceptr gpu_texture = this->camera_textures[i]->ExportAsCuda();

        camera_textures.push_back(gpu_texture);
    }

    return camera_textures;
}
