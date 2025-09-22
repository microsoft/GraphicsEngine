#include "Engine.h"
#include <iostream>
#include <fstream>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    std::ofstream file;
    file.open ("cout.txt");
    std::streambuf* sbuf = std::cout.rdbuf();
    std::cout.rdbuf(file.rdbuf());

    Engine engine(hInstance, width, height);
	Model model;
    engine.Init();
    engine.Run();

    std::cout.rdbuf(sbuf);
    file.close();
    return 0;
}