
#include "VulkanPipeline.h"

VulkanPipeline::VulkanPipeline(int camera_count) {
    int ui_texture_count = 3;
    int descriptor_count = (3 * camera_count) + ui_texture_count;

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
        VulkanTexture* depth_texture = new VulkanTexture(this->vulkan_core);
        VulkanTexture* phash_texture = new VulkanTexture(this->vulkan_core);
        VulkanTexture* centroid_texture = new VulkanTexture(this->vulkan_core);

        result = depth_texture->LoadImage("data/media/viewport_default.jpg");
        result = phash_texture->LoadImage("data/media/phash_default.jpg");
        result = centroid_texture->LoadImage("data/media/phash_default.jpg");

        this->depth_textures.push_back(depth_texture);
        this->phash_textures.push_back(phash_texture);
        this->centroid_textures.push_back(centroid_texture);

        this->texture_list.push_back(depth_texture);
        this->texture_list.push_back(phash_texture);
        this->texture_list.push_back(centroid_texture);
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

std::vector<CUdeviceptr> VulkanPipeline::GetDepthTextures() {
    std::vector<CUdeviceptr> textures{};

    for (int i = 0; i < this->depth_textures.size(); i++) {
        CUdeviceptr gpu_texture = this->depth_textures[i]->ExportAsCuda();

        textures.push_back(gpu_texture);
    }

    return textures;
}

std::vector<CUdeviceptr> VulkanPipeline::GetPhashTextures() {
    std::vector<CUdeviceptr> textures{};

    for (int i = 0; i < this->depth_textures.size(); i++) {
        CUdeviceptr gpu_texture = this->phash_textures[i]->ExportAsCuda();

        textures.push_back(gpu_texture);
    }

    return textures;
}

std::vector<CUdeviceptr> VulkanPipeline::GetCentroidTextures() {
    std::vector<CUdeviceptr> textures{};

    for (int i = 0; i < this->centroid_textures.size(); i++) {
        CUdeviceptr gpu_texture = this->centroid_textures[i]->ExportAsCuda();

        textures.push_back(gpu_texture);
    }

    return textures;
}
