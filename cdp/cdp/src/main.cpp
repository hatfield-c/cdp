#include "ui/MainGui.h"

int main(int, char**)
{
    MainGui* main_gui = new MainGui();

    while (!main_gui->IsWindowClosed()) {
        main_gui->Update();
    }

    return 0;
}
