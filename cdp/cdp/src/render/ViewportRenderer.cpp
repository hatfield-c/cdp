#include "ViewportRenderer.h"

ViewportRenderer::ViewportRenderer(CUdeviceptr viewport_image) {
    this->N = 5;
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
    RenderViewport(this->viewport_image, this->image_cuda, 5);
    //RenderViewport(this->image_cuda, 5);

    // Copy result from device memory to host memory
    // h_C contains the result in host memory
    cudaMemcpy(this->image, this->image_cuda, this->size, cudaMemcpyDeviceToHost);

    std::cout << "test\n";
    printf("%u\n", this->image[0]);
    printf("%u\n", this->image[1]);
    printf("%u\n", this->image[2]);
    printf("%u\n", this->image[3]);
    printf("%u\n", this->image[4]);
}

void ViewportRenderer::Cleanup() {
    // Free device memory
    cudaFree(this->image_cuda);

    // Free host memory
    free(this->image);
}