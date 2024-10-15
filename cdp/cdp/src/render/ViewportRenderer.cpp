#include "ViewportRenderer.h"

ViewportRenderer::ViewportRenderer(CUdeviceptr viewport_image) {
    this->N = 32;
    this->size = this->N * sizeof(float);

    // Allocate input vectors h_A
    byte* h_A = (byte*)malloc(this->size);

    // Initialize input vectors
    // ...

    h_A[2] = 2;

    // Allocate vectors in device memory
    byte* d_A;
    cudaMalloc(&d_A, this->size);

    // Copy vectors from host memory to device memory
    cudaMemcpy(d_A, h_A, this->size, cudaMemcpyHostToDevice);

    this->image = h_A;
    this->image_cuda = d_A;

    this->viewport_image = viewport_image;
}

void ViewportRenderer::Render() {
    RenderViewport(this->viewport_image, this->image_cuda, 1);
    //RenderViewport(this->image_cuda, 5);

    // Copy result from device memory to host memory
    // h_C contains the result in host memory
    cudaMemcpy(this->image, this->image_cuda, this->size, cudaMemcpyDeviceToHost);

    for (int i = 0; i < 32; i++) {
        printf("%d: %u\n", i, this->image[i]);
    }
    
}

void ViewportRenderer::Cleanup() {
    // Free device memory
    cudaFree(this->image_cuda);

    // Free host memory
    free(this->image);
}