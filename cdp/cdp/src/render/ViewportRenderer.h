#pragma once

#include <iostream>

#include "CudaRender.cuh"

class ViewportRenderer {
	public:
		int N;
		size_t size;
		float* image;
		float* image_cuda;

		ViewportRenderer();

		void Render();
		void Cleanup();
};