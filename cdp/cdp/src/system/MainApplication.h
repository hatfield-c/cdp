#pragma once

#include <stdlib.h>
#include <vector>
#include "cuda.h"

#include "../ui/MainGui.h"
#include "../engine/CudaEngine.h"

class MainApplication {
public:
	MainGui* main_gui;
	CudaEngine* engine;

	MainApplication();
	void Run();
};