#pragma once

#include <windows.h>
#include "Renderer.h"

class Engine
{
public:
    Engine(HINSTANCE hInstance, int width, int height);
    ~Engine();

    void Init();
    void Run();
    void Cleanup();

private:
    void InitWindow();

    HINSTANCE hInstance;
    HWND hwnd;
    int width, height;

    Renderer* renderer;
};

