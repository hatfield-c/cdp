#define STB_IMAGE_IMPLEMENTATION

#include "ui/MainGui.h"
#include "Tester.cuh"

#include <stdlib.h>


// Host code

int main()
{
    int N = 5;
    size_t size = N * sizeof(float);

    // Allocate input vectors h_A and h_B in host memory
    float* h_A = (float*)malloc(size);
    float* h_B = (float*)malloc(size);
    float* h_C = (float*)malloc(size);

    // Initialize input vectors
    // ...

    h_A[2] = 2;
    h_B[2] = -3;

    // Allocate vectors in device memory
    float* d_A;
    cudaMalloc(&d_A, size);
    float* d_B;
    cudaMalloc(&d_B, size);
    float* d_C;
    cudaMalloc(&d_C, size);

    // Copy vectors from host memory to device memory
    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);

    /*
    // Invoke kernel
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
    VecAdd<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, N);
    */
    VecAddWrapper(d_A, d_B, d_C, N);

    // Copy result from device memory to host memory
    // h_C contains the result in host memory
    cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);

    std::cout << "test\n";
    printf("%f\n", h_C[2]);

    // Free device memory
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    // Free host memory
    // ...
    free(h_A);
    free(h_B);
    free(h_C);
}

/*
#define STB_IMAGE_IMPLEMENTATION

#include "ui/MainGui.h"

int main(int, char**)
{
    MainGui* main_gui = new MainGui();

    while (!main_gui->IsWindowClosed()) {
        main_gui->Update();
    }

    main_gui->Cleanup();

    return 0;
}
*/