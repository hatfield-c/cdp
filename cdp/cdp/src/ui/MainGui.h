#pragma once

#include <iostream>
#include <string>
#include <format>
#include <vector>

#include "imgui_internal.h"

#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanTexture.h"

#include "../render/ViewportRenderer.h"

class MainGui {
	public:
		bool is_simulating = false;
		int camera_index = 0;
		int camera_count = 0;

		std::vector<std::string> camera_labels{};

		bool is_checked0 = false;
		bool is_checked1 = false;
		bool is_checked2 = false;
		float slider0 = 0;
		float slider1 = 0.5f;
		float slider2 = 1.0f;

		float uv_offset = 0.0f;
		float uv_delta = 0.00005f;

		ImVec4 desktop_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImVec4 color0 = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		ImVec4 color1 = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);
		ImVec4 color2 = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

		VulkanPipeline* vulkan_pipeline;

		MainGui(int camera_count);
		void Update();
		void Cleanup();
		bool IsWindowClosed();
		void DrawBackground();
		void DrawViewport();
		void DrawInspector();
		void ToggleButton(const char* str_id, const char* label, bool* value);
		void DrawCameraSelector();
};