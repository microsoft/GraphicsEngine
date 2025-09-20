#pragma once

#include <windows.h>
#include <vector>
#include "Renderer.h"
#include "Model.h"
#include "Audio.h"
#include "File.h"
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

    bool toggleClickCamera = false; // Toggle for click to rotate camera

private:
    void InitWindow();

    HINSTANCE hInstance;
    HWND hwnd;
    int width, height;

    AudioPlayer* audioPlayer;


	std::vector<Model*> models;

    std::vector<std::vector<float>> modelPos;
};

