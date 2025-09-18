#include "Engine.h"
#include <iostream>
#include <fstream>

//#pragma comment(lib, "d3d12.lib")
//#pragma comment(lib, "dxgi.lib")
//#pragma comment(lib, "d3dcompiler.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    std::ofstream file;
    file.open ("cout.txt");
    std::streambuf* sbuf = std::cout.rdbuf();
    std::cout.rdbuf(file.rdbuf());

    Engine engine(hInstance, 800, 800);
	Model model;
    engine.Init();
    engine.Run();

    std::cout.rdbuf(sbuf);
    file.close();
    return 0;
}