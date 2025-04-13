#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "driver_types.h"
#include <cuda.h>
#include <cuda_runtime.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <windows.h>
#include <fileapi.h>
#include <errhandlingapi.h>

#include "stb_image.h"

#include "VulkanCore.h"
#include "../../system/CudaError.h"

struct VulkanTexture {
	CUdeviceptr cuda_memory_address;

	VkDescriptorSet instance_descriptor;
	int width;
	int height;
	int channels = 4;
	VkDeviceSize memory_size;

	VkImageView image_view;
	VkImage vulkan_image;
	VkDeviceMemory image_memory;
	VkSampler sampler;
	VkBuffer upload_buffer;
	VkDeviceMemory upload_buffer_memory;
	VkResult error;
	VulkanCore* vulkan_core;

    void Init(VulkanCore* vulkan_core) {
        this->vulkan_core = vulkan_core;
    }

    bool LoadImage(const char* filename) {
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

    void AllocateImage() {
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
        info.tiling = VK_IMAGE_TILING_LINEAR;
        info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkExternalMemoryImageCreateInfo vkExternalMemImageCreateInfo = {};
        vkExternalMemImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        vkExternalMemImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        vkExternalMemImageCreateInfo.pNext = NULL;

        info.pNext = &vkExternalMemImageCreateInfo;

        this->error = vkCreateImage(this->vulkan_core->g_Device, &info, this->vulkan_core->g_Allocator, &this->vulkan_image);
        this->vulkan_core->check_vk_result(this->error);

        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(this->vulkan_core->g_Device, this->vulkan_image, &req);
        //this->memory_size = req.size;

        WindowsSecurityAttributes win_security_attributes = WindowsSecurityAttributes();
        VkExportMemoryWin32HandleInfoKHR vulkanExportMemoryWin32HandleInfoKHR{ };
        vulkanExportMemoryWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
        vulkanExportMemoryWin32HandleInfoKHR.pNext = NULL;
        //vulkanExportMemoryWin32HandleInfoKHR.pAttributes = &win_security_attributes;
        vulkanExportMemoryWin32HandleInfoKHR.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
        vulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)NULL;

        VkExportMemoryAllocateInfoKHR vulkanExportMemoryAllocateInfoKHR = {};
        vulkanExportMemoryAllocateInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
        vulkanExportMemoryAllocateInfoKHR.pNext = &vulkanExportMemoryWin32HandleInfoKHR;
        vulkanExportMemoryAllocateInfoKHR.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = this->FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        alloc_info.pNext = &vulkanExportMemoryAllocateInfoKHR;

        VkMemoryRequirements vkMemoryRequirements = {};
        vkGetImageMemoryRequirements(this->vulkan_core->g_Device, this->vulkan_image, &vkMemoryRequirements);
        this->memory_size = vkMemoryRequirements.size;

        this->error = vkAllocateMemory(this->vulkan_core->g_Device, &alloc_info, this->vulkan_core->g_Allocator, &this->image_memory);
        this->vulkan_core->check_vk_result(this->error);
        this->error = vkBindImageMemory(this->vulkan_core->g_Device, this->vulkan_image, this->image_memory, 0);
        this->vulkan_core->check_vk_result(this->error);
    }

    CUdeviceptr ExportAsCuda() {
        PFN_vkGetMemoryWin32HandleKHR fpGetMemoryWin32HandleKHR;
        fpGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(this->vulkan_core->g_Instance, "vkGetMemoryWin32HandleKHR");

        HANDLE handle;

        VkMemoryGetWin32HandleInfoKHR vkMemoryGetWin32HandleInfoKHR = {};
        vkMemoryGetWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        vkMemoryGetWin32HandleInfoKHR.pNext = NULL;
        vkMemoryGetWin32HandleInfoKHR.memory = this->image_memory;
        vkMemoryGetWin32HandleInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        this->error = fpGetMemoryWin32HandleKHR(this->vulkan_core->g_Device, &vkMemoryGetWin32HandleInfoKHR, &handle);
        this->vulkan_core->check_vk_result(this->error);

        cudaExternalMemory_t cudaExtMemImageBuffer;
        cudaExternalMemoryHandleDesc cudaExtMemHandleDesc;
        memset(&cudaExtMemHandleDesc, 0, sizeof(cudaExtMemHandleDesc));

        cudaExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
        cudaExtMemHandleDesc.handle.win32.handle = handle;
        cudaExtMemHandleDesc.size = this->memory_size;

        CudaError::CheckError((cudaError_enum)cudaImportExternalMemory(&cudaExtMemImageBuffer, &cudaExtMemHandleDesc), __FILE__, __LINE__);

        void* cuda_memory_pointer = NULL;

        cudaExternalMemoryBufferDesc buffer_description;
        buffer_description.flags = 0; // must be zero
        buffer_description.offset = 0;
        buffer_description.size = this->memory_size;

        CudaError::CheckError((cudaError_enum)cudaExternalMemoryGetMappedBuffer(&cuda_memory_pointer, cudaExtMemImageBuffer, &buffer_description), __FILE__, __LINE__);

        CloseHandle(handle);

        return (CUdeviceptr)cuda_memory_pointer;
    }

    void CreateImageView() {
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

    void CreateSampler() {
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_NEAREST;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;
        sampler_info.maxAnisotropy = 0.0f;
        this->error = vkCreateSampler(this->vulkan_core->g_Device, &sampler_info, this->vulkan_core->g_Allocator, &this->sampler);
        this->vulkan_core->check_vk_result(this->error);
    }

    void AllocateBuffer(size_t image_size) {
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

    void UploadToBuffer(size_t image_size, unsigned char* image_data) {
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

    VkCommandBuffer CreateCommandBuffer() {
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

    void SendCopyImageCommand(VkCommandBuffer command_buffer) {
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

    void CloseCommandBuffer(VkCommandBuffer command_buffer) {
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

    void RemoveTexture()
    {
        vkFreeMemory(this->vulkan_core->g_Device, this->upload_buffer_memory, nullptr);
        vkDestroyBuffer(this->vulkan_core->g_Device, this->upload_buffer, nullptr);
        vkDestroySampler(this->vulkan_core->g_Device, this->sampler, nullptr);
        vkDestroyImageView(this->vulkan_core->g_Device, this->image_view, nullptr);
        vkDestroyImage(this->vulkan_core->g_Device, this->vulkan_image, nullptr);
        vkFreeMemory(this->vulkan_core->g_Device, this->image_memory, nullptr);
    }

    uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(this->vulkan_core->g_PhysicalDevice, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        return 0xFFFFFFFF; // Unable to find memoryType
    }
};