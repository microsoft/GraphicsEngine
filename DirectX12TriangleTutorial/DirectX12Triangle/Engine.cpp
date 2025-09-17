#include "Engine.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
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

    HWND hwnd = CreateWindow(wc.lpszClassName, L"DirectX Engine", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);
    
    ShowWindow(hwnd, SW_SHOW);
}

void Engine::Init() {
    InitWindow();
    renderer = new Renderer(hwnd, width, height);
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