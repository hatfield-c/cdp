#include "ViewportRenderer.h"

ViewportRenderer::ViewportRenderer() {
    this->N = 5;
    this->size = this->N * sizeof(float);

    // Allocate input vectors h_A
    float* h_A = (float*)malloc(this->size);

    // Initialize input vectors
    // ...

    h_A[2] = 2;

    // Allocate vectors in device memory
    float* d_A;
    cudaMalloc(&d_A, this->size);

    // Copy vectors from host memory to device memory
    cudaMemcpy(d_A, h_A, this->size, cudaMemcpyHostToDevice);

    this->image = h_A;
    this->image_cuda = d_A;
}

void ViewportRenderer::Render() {
    
    RenderViewport(this->image_cuda, 5);

    // Copy result from device memory to host memory
    // h_C contains the result in host memory
    cudaMemcpy(this->image, this->image_cuda, this->size, cudaMemcpyDeviceToHost);

    std::cout << "test\n";
    printf("%f\n", this->image[2]);
}

void ViewportRenderer::Cleanup() {
    // Free device memory
    cudaFree(this->image_cuda);

    // Free host memory
    free(this->image);
}