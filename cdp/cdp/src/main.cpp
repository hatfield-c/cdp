#define STB_IMAGE_IMPLEMENTATION
#define VK_USE_PLATFORM_WIN32_KHR

#include "system/MainApplication.h"

int main(int, char**) {

    MainApplication app = MainApplication();
    app.Run();

    return 0;
}

