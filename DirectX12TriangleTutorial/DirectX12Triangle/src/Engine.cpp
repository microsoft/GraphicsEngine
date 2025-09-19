#include "Engine.h"

static Engine * engine = nullptr;

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
        case 'W':
            engine->renderer->HandleForward(1.0f);
            return 0;
        case 'A':
            engine->renderer->HandleX(-1.0f);
            return 0;
        case 'S':
            engine->renderer->HandleForward(-1.0f);
            return 0;
        case 'D':
            engine->renderer->HandleX(1.0f);
            return 0;
        case 'M':
            engine->renderer->HandleMouseMove(0.5f, engine->deltaX);
            return 0;
        case 'N':
            engine->renderer->HandleMouseMove(-0.5f, -1 * engine->deltaX);
            return 0;
        }
    }
    else if (message == WM_LBUTTONUP) {
        engine->firstMouse = true;
    }
    else if (message == WM_MOUSEMOVE) {
        if (wParam && MK_RBUTTON) {
            
            //engine->firstMouse = true;

            POINT currentPos;

            GetCursorPos(&currentPos);

            if (engine->firstMouse) {
                engine->lastPos = currentPos;
                engine->firstMouse = false;
            }

            float deltaX = static_cast<float>(currentPos.x - engine->lastPos.x);
            float deltaY = static_cast<float>(currentPos.y - engine->lastPos.y);

            std::string test = "deltaX: " + std::to_string(deltaX) + "\n";
            OutputDebugStringA(test.c_str());

            test = "deltaY: " + std::to_string(deltaY);
            OutputDebugStringA(test.c_str());

            engine->lastPos = currentPos;

            if (engine) {
                engine->renderer->HandleMouseMove(deltaX, deltaY);
            }

        }/*
        else {
            engine->firstMouse = true;
        }*/
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

Engine::Engine(HINSTANCE hInstance, int width, int height) 
    : hInstance(hInstance), hwnd(nullptr), width(width), height(height), renderer(nullptr)
{
    engine = this;
}

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