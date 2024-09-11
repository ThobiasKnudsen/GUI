#include <webgpu/webgpu_cpp.h>

#include <cstdlib>
#include <iostream>


int main(int argc, char *argv[]) {

    UI ui;
    ui.Initialize(800, 600, "WebGPU UI");
    while(ui.IsRunning()) {
        ui.MainLoop();
    }
    ui.Terminate();

    return 0;
}
