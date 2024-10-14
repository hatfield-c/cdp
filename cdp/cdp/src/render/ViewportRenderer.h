#pragma once

#include <iostream>

#include "../ui/vulkan/VulkanTexture.h"
#include "CudaRender.cuh"

class ViewportRenderer {
	public:
		CUdeviceptr viewport_image;

		int N;
		size_t size;
		byte* image;
		byte* image_cuda;

		ViewportRenderer(CUdeviceptr viewport_image);

		void Render();
		void Cleanup();
};