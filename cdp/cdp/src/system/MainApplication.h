#pragma once

#include <stdlib.h>
#include <vector>
#include "cuda.h"

#include "../ui/MainGui.h"
#include "../ui/GuiData.h"
#include "../engine/CudaEngine.h"

class MainApplication {
public:
	MainGui* main_gui;
	CudaEngine* engine;

	MainApplication();
	void Run();
	void GuiAction(GuiData gui_data);
};