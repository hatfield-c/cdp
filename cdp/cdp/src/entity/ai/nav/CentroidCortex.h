#pragma once

#include <iostream>
#include <string>

#include "../../../engine/Transform.h"
#include "../../../engine/Indexer.h"

struct CentroidCortex {
	Vector2 phash_size{ 16, 16 };
	unsigned long long centroid_count = 256;
	unsigned long long phash_count = 256;
	unsigned long long data_count = 256 * 256;
	unsigned long long data_size = 256 * 256 * sizeof(float);
	float* centroids_cpu;
	float* centroids;

	void Init() {
		std::string centroid_path = "data/nav/centroids/centroids.float";

		this->centroids_cpu = new float[data_count];
		
		FILE* in_file;
		fopen_s(&in_file, centroid_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", centroid_path.c_str());
			exit(1);
		}
		int result = (int)fread(this->centroids_cpu, sizeof(float), (size_t)this->data_count, in_file);
		fclose(in_file);

		cudaMalloc(&this->centroids, this->data_size);
		CudaError::CheckError((cudaError_enum)cudaMemcpy(this->centroids, this->centroids_cpu, this->data_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);

		printf("Centroids loaded:\n    0  : ");

		for (unsigned long long i = 128; i < 132; i++) {
			unsigned long long index = Indexer::FlatIndex2(i, 0, this->phash_count);
			printf("%.2f, ", this->centroids_cpu[index]);
		}
		printf("\n    1  : ");
		for (unsigned long long i = 128; i < 132; i++) {
			unsigned long long index = Indexer::FlatIndex2(i, 1, this->phash_count);
			printf("%.2f, ", this->centroids_cpu[index]);
		}
		printf("\n    255: ");
		for (unsigned long long i = 128; i < 132; i++) {
			unsigned long long index = Indexer::FlatIndex2(i, 255, this->phash_count);
			printf("%.2f, ", this->centroids_cpu[index]);
		}
		printf("\n");
	}

	void Update(float* in_data) {

	}

};