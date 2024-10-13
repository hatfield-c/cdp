#pragma once

#include <iostream>

#include "../ui/vulkan/VulkanTexture.h"
#include "CudaRender.cuh"

class ViewportRenderer {
	public:
		VulkanTexture* viewport_image;

		int N;
		size_t size;
		float* image;
		float* image_cuda;

		ViewportRenderer(VulkanTexture* viewport_image);

		void Render();
		void Cleanup();
};