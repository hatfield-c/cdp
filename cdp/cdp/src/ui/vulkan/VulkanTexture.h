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

class VulkanTexture {
	public:
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

		VulkanTexture(VulkanCore* vulkan_core);

		bool LoadImage(const char* filename);
		void AllocateImage();
		void CreateImageView();
		void CreateSampler();
		void AllocateBuffer(size_t image_size);
		void UploadToBuffer(size_t image_size, unsigned char* image_data);
		VkCommandBuffer CreateCommandBuffer();
		void SendCopyImageCommand(VkCommandBuffer command_buffer);
		void CloseCommandBuffer(VkCommandBuffer command_buffer);
		uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
		void RemoveTexture();
		CUdeviceptr ExportAsCuda();
};