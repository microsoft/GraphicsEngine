#pragma once

#include <windows.h>
#include <vector>
#include "Renderer.h"
#include "Model.h"
#include <filesystem>

class Engine
{
public:
    Engine(HINSTANCE hInstance, int width, int height);
    ~Engine();

    void Init();
    void Run();
    void Cleanup();

    Renderer* renderer;

    bool firstMouse = true;
    POINT lastPos = { 0, 0 };

    float deltaX = 0.9f;

private:
    void InitWindow();

    HINSTANCE hInstance;
    HWND hwnd;
    int width, height;

	std::vector<Model*> models;
};

