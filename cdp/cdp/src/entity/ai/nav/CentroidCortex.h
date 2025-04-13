#pragma once

#include <string>

#include "../../../engine/Transform.h"
#include "../../../engine/Indexer.h"

struct CentroidCortex {
	Vector2 phash_size{ 16, 16 };
	unsigned long long centroid_count = 256;
	unsigned long long phash_count = 256;
	unsigned long long data_count = 256 * 256;
	unsigned long long data_size = 256 * 256 * sizeof(float);
	float* centroids;

	void Init() {
		std::string centroid_path = "data/polyfield/wi.float";

		this->centroids = new float[centroid_count];
		
		FILE* in_file;
		fopen_s(&in_file, centroid_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", centroid_path.c_str());
			exit(1);
		}
		int result = (int)fread(this->centroids, sizeof(float), (size_t)this->data_count, in_file);
		fclose(in_file);

		printf("Centroids loaded.\n");

		for (unsigned long long i = 0; i < 4; i++) {
			unsigned long long index = Indexer::FlatIndex2(i, 0, this->phash_count);
			printf("%.2f, ", this->centroids[index]);
		}
		printf("\n");
		for (unsigned long long i = 0; i < 4; i++) {
			unsigned long long index = Indexer::FlatIndex2(i, 1, this->phash_count);
			printf("%.2f, ", this->centroids[index]);
		}
		printf("\n");
		for (unsigned long long i = 0; i < 4; i++) {
			unsigned long long index = Indexer::FlatIndex2(i, 255, this->phash_count);
			printf("%.2f, ", this->centroids[index]);
		}
		printf("\n");
		exit(0);
	}

	void Update(float* in_data) {

	}

};