#pragma once

#include <windows.h>
#include <vector>
#include "Renderer.h"
#include "Model.h"

class Engine
{
public:
    Engine(HINSTANCE hInstance, int width, int height);
    ~Engine();

    void Init();
    void Run();
    void Cleanup();

    Renderer* renderer;

private:
    void InitWindow();

    HINSTANCE hInstance;
    HWND hwnd;
    int width, height;

	std::vector<Model> models;

};

