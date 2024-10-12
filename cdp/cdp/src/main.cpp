#define STB_IMAGE_IMPLEMENTATION
#define VK_USE_PLATFORM_WIN32_KHR

#include "ui/MainGui.h"

#include <stdlib.h>

int main(int, char**)
{
    MainGui* main_gui = new MainGui();

    while (!main_gui->IsWindowClosed()) {
        main_gui->Update();
    }

    main_gui->Cleanup();

    return 0;
}

