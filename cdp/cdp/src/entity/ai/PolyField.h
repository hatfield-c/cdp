#pragma once

#include "../../engine/Transform.h"
#include "../../engine/Indexer.h"

struct PolyField {
	unsigned long long in_nodes;
	unsigned long long h_nodes;
	unsigned long long out_nodes;
	unsigned long long h_depth;
	unsigned long long total_depth;

	unsigned long long wi_size;
	unsigned long long wh_size;
	unsigned long long wo_size;
	unsigned long long bi_size;
	unsigned long long bh_size;
	unsigned long long bo_size;

	float* wi;
	float* wh;
	float* wo;
	float* bi;
	float* bh;
	float* bo;

	float* buffer;

	void Init(unsigned long long in_nodes, unsigned long long h_nodes, unsigned long long out_nodes, unsigned long long h_depth) {
		//float* wi;
		//float* wh;
		//float* wo;
		//float* bi;
		//float* bh;
		//float* bo;

		std::string wi_path = "data/polyfield/wi.float";
		std::string wh_path = "data/polyfield/wh.float";
		std::string wo_path = "data/polyfield/wo.float";
		std::string bi_path = "data/polyfield/bi.float";
		std::string bh_path = "data/polyfield/bh.float";
		std::string bo_path = "data/polyfield/bo.float";

		this->in_nodes = in_nodes;
		this->h_nodes = h_nodes;
		this->out_nodes = out_nodes;
		this->h_depth = h_depth;
		this->total_depth = h_depth + 2;

		this->wi_size = in_nodes * h_nodes;
		this->wh_size = h_nodes * h_nodes * h_depth;
		this->wo_size = out_nodes * h_nodes;
		this->bi_size = h_nodes;
		this->bh_size = h_nodes;
		this->bo_size = out_nodes;

		this->buffer = new float[h_nodes * h_nodes];

		this->wi = new float[this->wi_size];
		this->wh = new float[this->wh_size];
		this->wo = new float[this->wo_size];
		this->bi = new float[this->bi_size];
		this->bh = new float[this->bh_size];
		this->bo = new float[this->bo_size];
		//cudaMalloc(&this->wi, (size_t)this->wi_size * sizeof(float));
		//cudaMalloc(&this->wh, (size_t)this->wh_size * sizeof(float));
		//cudaMalloc(&this->wo, (size_t)this->wo_size * sizeof(float));
		//cudaMalloc(&this->bi, (size_t)this->bi_size * sizeof(float));
		//cudaMalloc(&this->bh, (size_t)this->bh_size * sizeof(float));
		//cudaMalloc(&this->bo, (size_t)this->bo_size * sizeof(float));
		//cudaMalloc(&this->buffer, (size_t)(h_nodes * sizeof(float)));

		FILE* in_file;
		fopen_s(&in_file, wi_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", wi_path.c_str());
			exit(1);
		}
		int result = (int)fread(wi, sizeof(float), (size_t)this->wi_size, in_file);
		fclose(in_file);

		fopen_s(&in_file, wh_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", wh_path.c_str());
			exit(1);
		}
		result = (int)fread(wh, sizeof(float), (size_t)this->wh_size, in_file);
		fclose(in_file);

		fopen_s(&in_file, wo_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", wo_path.c_str());
			exit(1);
		}
		result = (int)fread(wo, sizeof(float), (size_t)this->wo_size, in_file);
		fclose(in_file);

		fopen_s(&in_file, bi_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", bi_path.c_str());
			exit(1);
		}
		result = (int)fread(bi, sizeof(float), (size_t)this->bi_size, in_file);
		fclose(in_file);

		fopen_s(&in_file, bh_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", bh_path.c_str());
			exit(1);
		}
		result = (int)fread(bh, sizeof(float), (size_t)this->bh_size, in_file);
		fclose(in_file);

		fopen_s(&in_file, bo_path.c_str(), "rb");
		if (in_file == NULL) {
			printf("\n\n[Warning] file did not open when loading:\n    %s!\n", bo_path.c_str());
			exit(1);
		}
		result = (int)fread(bo, sizeof(float), (size_t)this->bo_size, in_file);
		fclose(in_file);

		//cudaMemcpy(this->wi, wi, (size_t)(this->wi_size * sizeof(float)), cudaMemcpyHostToDevice);
		//cudaMemcpy(this->wh, wh, (size_t)(this->wi_size * sizeof(float)), cudaMemcpyHostToDevice);
		//cudaMemcpy(this->wo, wo, (size_t)(this->wi_size * sizeof(float)), cudaMemcpyHostToDevice);
		//cudaMemcpy(this->bi, bi, (size_t)(this->wi_size * sizeof(float)), cudaMemcpyHostToDevice);
		//cudaMemcpy(this->bh, bh, (size_t)(this->wi_size * sizeof(float)), cudaMemcpyHostToDevice);
		//cudaMemcpy(this->bo, bo, (size_t)(this->wi_size * sizeof(float)), cudaMemcpyHostToDevice);

		//free(wi);
		//free(wh);
		//free(wo);
		//free(bi);
		//free(bh);
		//free(bo);

		printf("PolyField loaded.\n");

		for (unsigned long long j = 0; j < 2; j++) {
			for (unsigned long long i = 0; i < 6; i++) {
				unsigned long long index = Indexer::FlatIndex3(i, j, 0, this->h_nodes, this->h_nodes);
				printf("%.2f, ", this->wh[index]);
			}
			printf("\n");
		}
		printf("\n");
		for (unsigned long long j = 0; j < 2; j++) {
			for (unsigned long long i = 0; i < 6; i++) {
				unsigned long long index = Indexer::FlatIndex3(i, j, 1, this->h_nodes, this->h_nodes);
				printf("%.2f, ", this->wh[index]);
			}
			printf("\n");
		}
		printf("\n");

		exit(0);
	}

	float ForwardPass(float* in_data) {

		for (int i = 0; i < this->h_nodes; i++) {
			float node_value = 0;
			float bias = this->bi[i];

			for (int j = 0; j < this->in_nodes; j++) {
				unsigned long long w_index = Indexer::FlatIndex2((unsigned long long)j, (unsigned long long)i, (unsigned long long)this->in_nodes);
				float weight = this->wi[w_index];
				float in_value = in_data[j];

				float connection_value = weight * in_value;
				node_value += connection_value;
			}

			node_value += bias;
			node_value = this->ReLU(node_value);

			this->buffer[i] = node_value;
		}

		for (int h = 0; h < this->h_depth; h++) {
			for (int i = 0; i < this->h_nodes; i++) {
				float node_value = 0;
				float bias = this->bi[i];

				for (int j = 0; j < this->in_nodes; j++) {
					unsigned long long w_index = Indexer::FlatIndex3((unsigned long long)h, (unsigned long long)j, (unsigned long long)i, (unsigned long long)this->h_nodes,(unsigned long long)this->in_nodes);
					float weight = this->wh[w_index];
					float in_value = in_data[j];

					float connection_value = weight * in_value;
					node_value += connection_value;
				}

				node_value += bias;
				node_value = this->ReLU(node_value);

				this->buffer[i] = node_value;
			}
		}
		
		float out_data = 0;
		/*for (int i = 0; i < this->h_size; i++) {
			float w_value = this->w1[i];
			float h_value = this->h[i];

			out_data += w_value * h_value;

		}
		out_data += this->b1[0];
		out_data = this->Sigmoid(out_data);
		*/
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