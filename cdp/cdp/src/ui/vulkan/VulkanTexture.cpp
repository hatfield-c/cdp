#include "VulkanTexture.h"

VulkanTexture::VulkanTexture(VulkanCore* vulkan_core) {
    this->vulkan_core = vulkan_core;
}

bool VulkanTexture::LoadImage(const char* filename) {
    unsigned char* image_data = stbi_load(filename, &this->width, &this->height, 0, this->channels);

    if (image_data == NULL)
        return false;

    size_t image_size = this->width * this->height * this->channels;

    this->AllocateImage();
    this->CreateImageView();
    this->CreateSampler();

    this->instance_descriptor = ImGui_ImplVulkan_AddTexture(this->sampler, this->image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    this->AllocateBuffer(image_size);
    this->UploadToBuffer(image_size, image_data);

    stbi_image_free(image_data);

    VkCommandBuffer command_buffer = this->CreateCommandBuffer();
    this->SendCopyImageCommand(command_buffer);
    this->CloseCommandBuffer(command_buffer);

    return true;
}

void VulkanTexture::AllocateImage() {
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = VK_FORMAT_R8G8B8A8_UNORM;
    info.extent.width = this->width;
    info.extent.height = this->height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    this->error = vkCreateImage(this->vulkan_core->g_Device, &info, this->vulkan_core->g_Allocator, &this->vulkan_image);
    this->vulkan_core->check_vk_result(this->error);

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(this->vulkan_core->g_Device, this->vulkan_image, &req);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = this->FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    this->error = vkAllocateMemory(this->vulkan_core->g_Device, &alloc_info, this->vulkan_core->g_Allocator, &this->image_memory);
    this->vulkan_core->check_vk_result(this->error);
    this->error = vkBindImageMemory(this->vulkan_core->g_Device, this->vulkan_image, this->image_memory, 0);
    this->vulkan_core->check_vk_result(this->error);
}

void VulkanTexture::CreateImageView() {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = this->vulkan_image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = VK_FORMAT_R8G8B8A8_UNORM;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.layerCount = 1;
    this->error = vkCreateImageView(this->vulkan_core->g_Device, &info, this->vulkan_core->g_Allocator, &this->image_view);
    this->vulkan_core->check_vk_result(this->error);
}

void VulkanTexture::CreateSampler() {
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.minLod = -1000;
    sampler_info.maxLod = 1000;
    sampler_info.maxAnisotropy = 1.0f;
    this->error = vkCreateSampler(this->vulkan_core->g_Device, &sampler_info, this->vulkan_core->g_Allocator, &this->sampler);
    this->vulkan_core->check_vk_result(this->error);
}

void VulkanTexture::AllocateBuffer(size_t image_size) {
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = image_size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    this->error = vkCreateBuffer(this->vulkan_core->g_Device, &buffer_info, this->vulkan_core->g_Allocator, &this->upload_buffer);
    this->vulkan_core->check_vk_result(this->error);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(this->vulkan_core->g_Device, this->upload_buffer, &req);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = this->FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    this->error = vkAllocateMemory(this->vulkan_core->g_Device, &alloc_info, this->vulkan_core->g_Allocator, &this->upload_buffer_memory);
    this->vulkan_core->check_vk_result(this->error);
    this->error = vkBindBufferMemory(this->vulkan_core->g_Device, this->upload_buffer, this->upload_buffer_memory, 0);
    this->vulkan_core->check_vk_result(this->error);
}

void VulkanTexture::UploadToBuffer(size_t image_size, unsigned char* image_data) {
    void* map = NULL;
    this->error = vkMapMemory(this->vulkan_core->g_Device, this->upload_buffer_memory, 0, image_size, 0, &map);
    this->vulkan_core->check_vk_result(this->error);
    memcpy(map, image_data, image_size);
    VkMappedMemoryRange range[1] = {};
    range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range[0].memory = this->upload_buffer_memory;
    range[0].size = image_size;
    this->error = vkFlushMappedMemoryRanges(this->vulkan_core->g_Device, 1, range);
    this->vulkan_core->check_vk_result(this->error);
    vkUnmapMemory(this->vulkan_core->g_Device, this->upload_buffer_memory);
}

VkCommandBuffer VulkanTexture::CreateCommandBuffer() {
    VkCommandPool command_pool = this->vulkan_core->g_MainWindowData.Frames[this->vulkan_core->g_MainWindowData.FrameIndex].CommandPool;
    VkCommandBuffer command_buffer;

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    this->error = vkAllocateCommandBuffers(this->vulkan_core->g_Device, &alloc_info, &command_buffer);
    this->vulkan_core->check_vk_result(this->error);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    this->error = vkBeginCommandBuffer(command_buffer, &begin_info);
    this->vulkan_core->check_vk_result(this->error);

    return command_buffer;
}

void VulkanTexture::SendCopyImageCommand(VkCommandBuffer command_buffer) {
    VkImageMemoryBarrier copy_barrier[1] = {};
    copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier[0].image = this->vulkan_image;
    copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_barrier[0].subresourceRange.levelCount = 1;
    copy_barrier[0].subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = this->width;
    region.imageExtent.height = this->height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(command_buffer, this->upload_buffer, this->vulkan_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier use_barrier[1] = {};
    use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier[0].image = this->vulkan_image;
    use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    use_barrier[0].subresourceRange.levelCount = 1;
    use_barrier[0].subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
}

void VulkanTexture::CloseCommandBuffer(VkCommandBuffer command_buffer) {
    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;
    this->error = vkEndCommandBuffer(command_buffer);
    this->vulkan_core->check_vk_result(this->error);
    this->error = vkQueueSubmit(this->vulkan_core->g_Queue, 1, &end_info, VK_NULL_HANDLE);
    this->vulkan_core->check_vk_result(this->error);
    this->error = vkDeviceWaitIdle(this->vulkan_core->g_Device);
    this->vulkan_core->check_vk_result(this->error);
}

void VulkanTexture::RemoveTexture()
{
    vkFreeMemory(this->vulkan_core->g_Device, this->upload_buffer_memory, nullptr);
    vkDestroyBuffer(this->vulkan_core->g_Device, this->upload_buffer, nullptr);
    vkDestroySampler(this->vulkan_core->g_Device, this->sampler, nullptr);
    vkDestroyImageView(this->vulkan_core->g_Device, this->image_view, nullptr);
    vkDestroyImage(this->vulkan_core->g_Device, this->vulkan_image, nullptr);
    vkFreeMemory(this->vulkan_core->g_Device, this->image_memory, nullptr);
}

uint32_t VulkanTexture::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(this->vulkan_core->g_PhysicalDevice, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;

    return 0xFFFFFFFF; // Unable to find memoryType
}