#include "Engine.h"

//#pragma comment(lib, "d3d12.lib")
//#pragma comment(lib, "dxgi.lib")
//#pragma comment(lib, "d3dcompiler.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    Engine engine(hInstance, 800, 800);
	Model model;
    engine.Init();
    engine.Run();
    return 0;
}