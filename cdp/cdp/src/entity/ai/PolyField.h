#pragma once

#include <torch/script.h>

#include "../../engine/Transform.h"
#include "../../engine/Indexer.h"

struct PolyField {
	void Init() {
		torch::jit::script::Module module;
		torch::Device device(torch::kCPU);
		module = torch::jit::load("C:/dev/projects/cdp/cdp/cdp/data/polyfield/model.pt", device);
	}
};