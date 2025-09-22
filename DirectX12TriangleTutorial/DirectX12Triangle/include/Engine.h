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
    void RemoveModel(Model* model);

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

	Model* herobrineModel = nullptr;

    // Avoid large herobrine teleport perf hit
    DWORD lastAssetRebuildMs = 0;
    void TryRebuildRendererAssets();

    DirectX::XMFLOAT3 playerSpawnPos { 20.0f, 5.0f, -60.0f }; // Must match Camera spawn in camerapos
    float playerSpawnSafeRadius = 40.0f; // Adjust as needed (covers large scaled trees)
    bool IsInsidePlayerSpawn(float x, float z) const {
        float dx = x - playerSpawnPos.x;
        float dz = z - playerSpawnPos.z;
        return (dx * dx + dz * dz) < (playerSpawnSafeRadius * playerSpawnSafeRadius);
    }

};

