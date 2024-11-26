#pragma once

#include <iostream>
#include <string>
#include <format>
#include <vector>

#include "imgui_internal.h"
#include "imfilebrowser.h"

#include "vulkan/VulkanPipeline.h"
#include "vulkan/VulkanTexture.h"

#include "../engine/Indexer.h"
#include "GuiData.h"

class MainGui {
	public:
		int render_texture = 0;
		int vote_threshold = 4;
		unsigned long long ihm_position_index = Indexer::FlatIndex4(0, 48, 4, 42, 24, 100, 30);

		int camera_index = 0;
		int camera_count = 0;
		bool is_alert = false;

		std::vector<std::string> camera_labels{};

		GuiData gui_data{};

		ImGui::FileBrowser load_env_dialog;
		ImGui::FileBrowser save_env_dialog;
		ImGui::FileBrowser save_ihm_dialog = ImGui::FileBrowser(ImGuiFileBrowserFlags_EnterNewFilename);;

		float uv_offset = 0.0f;
		float uv_delta = 0.00005f;

		VulkanPipeline* vulkan_pipeline;

		MainGui(int camera_count);
		void Update();
		void RefreshGuiData();
		void Cleanup();
		void DrawBackground();
		void DrawViewport();
		void DrawInspector();
		void ToggleButton(const char* str_id, const char* label, bool* value);
		void DrawCameraSelector();
		bool IsWindowClosed();
};