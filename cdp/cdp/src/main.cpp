#define STB_IMAGE_IMPLEMENTATION

#include "ui/MainGui.h"

int main(int, char**)
{
    MainGui* main_gui = new MainGui();

    while (!main_gui->IsWindowClosed()) {
        main_gui->Update();
    }

    main_gui->Cleanup();

    return 0;
}
