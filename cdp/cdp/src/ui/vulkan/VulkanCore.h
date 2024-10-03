#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

class VulkanCore {
	public:
		GLFWwindow* window = nullptr;
		ImGuiIO* io = nullptr;
		ImGui_ImplVulkanH_Window* wd = nullptr;
		VkResult err = VK_NOT_READY;

		VkAllocationCallbacks* g_Allocator = nullptr;
		VkInstance               g_Instance = VK_NULL_HANDLE;
		VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice                 g_Device = VK_NULL_HANDLE;
		uint32_t                 g_QueueFamily = (uint32_t)-1;
		VkQueue                  g_Queue = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
		VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

		ImGui_ImplVulkanH_Window g_MainWindowData;
		int                      g_MinImageCount = 2;
		bool                     g_SwapChainRebuild = false;

		const std::vector<const char*> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		VulkanCore();
		static void glfw_error_callback(int error, const char* description);
		static void check_vk_result(VkResult err);
		static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension);
		VkPhysicalDevice SetupVulkan_SelectPhysicalDevice();
		void SetupVulkan(ImVector<const char*> instance_extensions);
		void CreateInstance(ImVector<const char*> instance_extensions);
		void SetupDevice();
		void CreateDescriptorPool();
		bool CheckValidationLayerSupport();
		void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
};