#pragma once

#include"cuda.h"
#include "cuda_runtime.h"
#include <iostream>

struct CudaError {
	static void CheckError(cudaError_enum result, const char* file, int line) {
		if (result) {
			fprintf(
				stderr,
				"CUDA error in %s at line %d.\n    [Error:%d %s] %s\n",
				file,
				line,
				static_cast<unsigned int>(result),
				(const char*)cudaGetErrorName((cudaError_t)result),
				cudaGetErrorString((cudaError_t)result)
			);
			exit(EXIT_FAILURE);
		}
	}
};