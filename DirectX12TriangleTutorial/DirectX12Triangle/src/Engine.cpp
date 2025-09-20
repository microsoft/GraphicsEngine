#include "Engine.h"
#include <random>

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
        }
    }
    else if (message == WM_SETFOCUS)
    {
        // Hide cursor
        ShowCursor(FALSE);
        return 0;
    }
    else if (message == WM_LBUTTONUP) {
        engine->firstMouse = true;
    }
    else if (message == WM_MOUSEMOVE) {
        if (wParam && MK_RBUTTON) {
            POINT currentPos;

            GetCursorPos(&currentPos);

            if (engine->firstMouse) {
                engine->lastPos = currentPos;
                engine->firstMouse = false;
            }

            float deltaX = static_cast<float>(currentPos.x - engine->lastPos.x);
            float deltaY = static_cast<float>(currentPos.y - engine->lastPos.y);

            if (deltaX != 0.0f || deltaY != 0.0f) {
                std::string test = "deltaX: " + std::to_string(deltaX) + "\n";
                OutputDebugStringA(test.c_str());

                test = "deltaY: " + std::to_string(deltaY);
                OutputDebugStringA(test.c_str());

                engine->lastPos = currentPos;

                if (engine) {
                    engine->renderer->HandleMouseMove(deltaX, deltaY);
                }
            }
        }
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

    hwnd = CreateWindow(wc.lpszClassName, L"???", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);

    // Get icon path
	std::filesystem::path exePath = GetExecutablePath();
	std::filesystem::path iconFile = exePath / "assets" / "Window" / "minecraft.ico";
    
    // Convert to wide string properly
    std::wstring iconPath = iconFile.wstring();
    HICON hIcon = (HICON)LoadImageW(NULL, iconPath.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    
    if (hIcon) { 
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon); // Also set small icon
        std::wcout << L"Icon loaded successfully from: " << iconPath << std::endl;
    } else {
        DWORD error = GetLastError();
        std::wcout << L"Failed to load icon from: " << iconPath << L" Error: " << error << std::endl;
        MessageBoxW(hwnd, L"ico not found", L"ico not found", MB_OK | MB_ICONERROR);
    }

    ShowWindow(hwnd, SW_SHOW);
}

void Engine::Init() {
    InitWindow();
    
    audioPlayer = new AudioPlayer();
    
    // Play horror background music
	std::filesystem::path exePath = GetExecutablePath();
	std::filesystem::path soundFile = exePath / "assets" / "Audio" / "ambience.mp3";
    if (!audioPlayer->BGM(soundFile.string())) {
        std::cout << "Failed to load background music!" << std::endl;
    } else {
        audioPlayer->SetVolume(300); // 50% volume
    }

    // Load multiple models
    models.clear();
    
    std::vector<BoundingBox> placedBoundingBoxes;

    // Example: Load grassplane
    Model* grassplane = new Model();
    if (grassplane->LoadFromObj("grassplane.obj")) {
        std::cout << "Grassplane loaded: " << grassplane->GetNumVertices() << " vertices" << std::endl;
        grassplane->SetPosition(0.0f, 0.0f, 0.0f);
        grassplane->ApplyTransformation();
        models.push_back(grassplane);
    } else {
        std::cout << "Failed to load grassplane.obj" << std::endl;
        delete grassplane;
    }

    // Example: Load cube
    Model* cube = new Model();
    if (cube->LoadFromObj("cottage_obj.obj")) {
        std::cout << "Cube loaded: " << cube->GetNumVertices() << " vertices" << std::endl;
        cube->SetPosition(-10.0f, 0.0f, 0.0f);
        cube->SetRotation(0.0f, DirectX::XM_PIDIV2, 0.0f); // DirectX::XM_PIDIV4
        cube->SetScale(2.0f, 2.0f, 2.0f);
        cube->ApplyTransformation();
        models.push_back(cube);

		placedBoundingBoxes.push_back(cube->b);
    }
    else {
        std::cout << "Failed to load cube.obj" << std::endl;
        delete cube;
    }

    // randomly scatter trees
    int treeNum = 50;
    int maxAttempts = 100; // Maximum attempts to place each tree
    std::uniform_real_distribution<float> distX(-200.0f, 200.0f);
    std::uniform_real_distribution<float> distZ(-200.0f, 200.0f);
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int i = 0; i < treeNum; ++i) {
        Model* tree = new Model();
        if (tree->LoadFromObj("Mineways2Skfb.obj")) {
            bool placed = false;

            // Try to find a non-intersecting position
            for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                // Random position
                float x = distX(gen);
                float z = distZ(gen);
                float y = -15.0f; // Keep trees at ground level

                // Set position and scale, then apply transformation to compute bounding box
                tree->SetPosition(x, y, z);
                tree->SetScale(30.0f, 30.0f, 30.0f);
                tree->ApplyTransformation();

                // Check if this tree intersects with any already placed model
                bool intersects = false;
                for (const auto& existingBox : placedBoundingBoxes) {
                    if (tree->b.Intersects(existingBox, tree->b)) {
                        intersects = true;
                        break;
                    }
                }

                if (!intersects) {
                    // Successfully placed without intersection
                    models.push_back(tree);
                    placedBoundingBoxes.push_back(tree->b);
                    placed = true;
                    break;
                }
            }
        }
        else {
            std::cout << "Failed to load tree.obj" << std::endl;
            delete tree;
        }
    }

    // randomly scatter diamonds
    int diamondNum = 5;
    std::uniform_real_distribution<float> diamondX(-50.0f, 50.0f);
    std::uniform_real_distribution<float> diamondZ(-50.0f, 50.0f);
    for (int i = 0; i < diamondNum; ++i) {
        Model* diamond = new Model();
        diamond->isRemovable = true;
        if (diamond->LoadFromObj("diamond.obj")) {
            bool placed = false;

            // Try to find a non-intersecting position
            for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                // Random position
                float x = diamondX(gen);
                float z = diamondZ(gen);
                float y = 1.0f;

                // Set position and scale, then apply transformation to compute bounding box
                diamond->SetPosition(x, y, z);
                diamond->SetScale(30.0f, 30.0f, 30.0f);
				diamond->SetRotation(0.0f, DirectX::XM_PI/2.0f, 0.0f);
                diamond->ApplyTransformation();

                // Check if this diamond intersects with any already placed model
                bool intersects = false;
                for (const auto& existingBox : placedBoundingBoxes) {
                    if (diamond->b.Intersects(existingBox, diamond->b)) {
                        intersects = true;
                        break;
                    }
                }

                if (!intersects) {
                    // Successfully placed without intersection
                    models.push_back(diamond);
                    placedBoundingBoxes.push_back(diamond->b);
                    placed = true;
                    break;
                }
            }
        }
        else {
            std::cout << "Failed to load diamond.obj" << std::endl;
            delete diamond;
        }
    }
    
    std::cout << "Total models loaded: " << models.size() << std::endl;
    
    // Create renderer and bind all models
    renderer = new Renderer(hwnd, width, height);
    renderer->BindModels(models);
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
            if (!renderer->c.collectedDiamonds.empty()) {
                for (auto* diamond : renderer->c.collectedDiamonds) {
                    // Find and remove the diamond
                    auto it = std::find(models.begin(), models.end(), diamond);
                    if (it != models.end()) {
                        delete* it;
                        models.erase(it);
                    }
                }

                // Clear the collected diamonds list
                renderer->c.collectedDiamonds.clear();

                // Update renderer with new model list
                renderer->BindModels(models);
                renderer->CreateAssets();
                renderer->CreateTextureResources();
            }

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

    if (audioPlayer) {
        audioPlayer->Stop();
        delete audioPlayer;
        audioPlayer = nullptr;
    }
    
    // Clean up dynamically allocated models
    for (auto& model : models) {
        delete model;
    }
    models.clear();
}
