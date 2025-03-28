#pragma once

#include <fstream>

#include "../../Transform.h"
#include "../../Indexer.h"

struct NeuralGrid {
	float* w0 = new float[6 * 256];
	float* b0 = new float[256];
	float* w1 = new float[6 * 256];
	float* b1 = new float[1];
	float* h = new float[256];

	const int in_size = 6;
	const int h_size = 256;

	void Init() {
		const char* w0_path = "data/hitpoly/w0.float";
		const char* b0_path = "data/hitpoly/b0.float";
		const char* w1_path = "data/hitpoly/w1.float";
		const char* b1_path = "data/hitpoly/b1.float";

		FILE* in_file;
		fopen_s(&in_file, w0_path, "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", w0_path);
			exit(1);
		}
		int result = (int)fread(this->w0, sizeof(float), (size_t)(this->in_size * this->h_size), in_file);
		fclose(in_file);

		fopen_s(&in_file, b0_path, "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", b0_path);
			exit(1);
		}
		result = (int)fread(this->b0, sizeof(float), (size_t)this->h_size, in_file);
		fclose(in_file);

		fopen_s(&in_file, w1_path, "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", w1_path);
			exit(1);
		}
		result = (int)fread(this->w1, sizeof(float), (size_t)(this->in_size * this->h_size), in_file);
		fclose(in_file);

		fopen_s(&in_file, b1_path, "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", b1_path);
			exit(1);
		}
		result = (int)fread(this->b1, sizeof(float), (size_t)this->h_size, in_file);
		fclose(in_file);

		printf("HitPoly loaded.");

		for (int i = 0; i < this->in_size * 2; i++) {
			printf("%.2f, ", w0[i]);

			if (i == this->in_size - 1) {
				printf("\n");
			}
		}
		printf("\n\n");

		for (int i = 0; i < this->in_size * 2; i++) {
			int index = (this->in_size * this->h_size) - ((this->in_size * 2) - i);
			printf("%.2f, ", w0[index]);

			if (i == this->in_size - 1) {
				printf("\n");
			}
		}
		printf("\n\n");

		exit(0);
	}

	bool IsRelease(Vector3 drone_offset, Vector3 drone_velocity) {
		float out_value = this->ForwardPass(drone_offset, drone_velocity);

		printf("[Out]: %.2f\n", out_value);

		bool is_release = false;
		if (out_value > 0.95) {
			is_release = true;
		}

		return true;
	}

	float ForwardPass(Vector3 drone_offset, Vector3 drone_velocity) {
		float in_data[6] = {
			drone_offset.x,
			drone_offset.y,
			drone_offset.z,
			drone_velocity.x,
			drone_velocity.y,
			drone_velocity.z
		};

		for (int i = 0; i < this->h_size; i++) {
			float node_value = 0;
			float bias = this->b0[i];

			for (int j = 0; j < this->in_size; j++) {
				unsigned long long w_index = Indexer::FlatIndex2((unsigned long long)j, (unsigned long long)i, (unsigned long long)this->in_size);
				float weight = this->w0[w_index];
				float in_value = in_data[j];

				float connection_value = weight * in_value;
				node_value += connection_value;
			}

			node_value += bias;
			node_value = this->ReLU(node_value);

			this->h[i] = node_value;
		}

		float out_data = 0;
		for (int i = 0; i < this->h_size; i++) {
			float w_value = this->w1[i];
			float h_value = this->h[i];

			out_data += w_value * h_value;
		}
		out_data += this->b1[0];
		out_data = this->Sigmoid(out_data);

		return out_data;
	}

	float ReLU(float value) {
		if (value > 0) {
			return value;
		}

		return 0.0f;
	}

	float Sigmoid(float value) {
		return 1.0f / (1.0f + exp(-value));
	}
};