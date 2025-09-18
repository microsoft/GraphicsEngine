#include "Engine.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    else if (message == WM_KEYDOWN) {
        // Handle key press events here
        switch (wParam) {
        case VK_ESCAPE:
            // Exit on Escape key
            PostQuitMessage(0);
            return 0;
        case VK_SPACE:
            // Example: Handle spacebar
            // You could pass this to the renderer or handle it here
            return 0;
            // Add more key cases as needed
        case 'W':

            return 0;
        case 'A':

            return 0;
        case 'S':

            return 0;
        case 'D':
            // Example: Handle WASD keys for camera movement
            // You might want to store these in Engine class and pass to renderer
            return 0;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

Engine::Engine(HINSTANCE hInstance, int width, int height) 
    : hInstance(hInstance), hwnd(nullptr), width(width), height(height), renderer(nullptr)
{}

Engine::~Engine() {
	Cleanup();
}

void Engine::InitWindow() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DX12EngineClass";

    RegisterClass(&wc);

    hwnd = CreateWindow(wc.lpszClassName, L"DirectX Engine", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);
    
    ShowWindow(hwnd, SW_SHOW);
}

void Engine::Init() {
    InitWindow();
    models.resize(1);
    models[0].LoadFromObj("cottage_obj.obj");
    renderer = new Renderer(hwnd, width, height);
    renderer->BindModel(models[0]);
    renderer->Init();
}

void Engine::Run() {
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            renderer->Update();
            renderer->Render();
        }
    }
}

void Engine::Cleanup() {
    if (renderer) {
        delete renderer;
        renderer = nullptr;
    }
}