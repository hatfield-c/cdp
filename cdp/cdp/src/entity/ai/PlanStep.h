#pragma once

struct PlanStep {
	float wall_direction;
	const char* start_phash_path;
	const char* end_phash_path;

	float* start_phash_gpu;
	float* start_phash_cpu;
	float* end_phash_gpu;
	float* end_phash_cpu;

	Vector2 phash_size{ 16, 16 };
	int phash_count = 16 * 16;

	void Init() {
		int phash_memory_size = this->phash_count * sizeof(float);
		
		if (this->IsStartValid()) {
			this->start_phash_cpu = new float[this->phash_count];
			memset(this->start_phash_cpu, 0, phash_memory_size);

			FILE* in_file;
			fopen_s(&in_file, this->start_phash_path, "rb");
			if (in_file == NULL) {
				printf("\n\n[Warning] .phash file did not open when loading:\n    %s!\n", start_phash_path);
				exit(1);
			}
			int result = fread(this->start_phash_cpu, sizeof(float), this->phash_count, in_file);
			fclose(in_file);

			CudaError::CheckError((cudaError_enum)cudaMalloc(&this->start_phash_gpu, phash_memory_size), __FILE__, __LINE__);
			CudaError::CheckError((cudaError_enum)cudaMemcpy(this->start_phash_gpu, this->start_phash_cpu, phash_memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
		}

		if (this->IsEndValid()) {
			this->end_phash_cpu = new float[this->phash_count];

			FILE* in_file;
			fopen_s(&in_file, this->end_phash_path, "rb");
			if (in_file == NULL) {
				printf("\n\n[Warning] .phash file did not open when loading:\n    %s!\n", end_phash_path);
				exit(1);
			}
			int result = fread(this->end_phash_cpu, sizeof(float), this->phash_count, in_file);
			fclose(in_file);

			CudaError::CheckError((cudaError_enum)cudaMalloc(&this->end_phash_gpu, phash_memory_size), __FILE__, __LINE__);
			CudaError::CheckError((cudaError_enum)cudaMemcpy(this->end_phash_gpu, this->end_phash_cpu, phash_memory_size, cudaMemcpyHostToDevice), __FILE__, __LINE__);
		}
	}

	bool IsStartValid() {
		if (this->start_phash_path[0] == '\0') {
			return false;
		}

		return true;
	}

	bool IsEndValid() {
		if (this->end_phash_path[0] == '\0') {
			return false;
		}

		return true;
	}
};