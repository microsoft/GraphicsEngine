#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <vector>
#include <Model.h>

class Camera
{
public:
    Camera();
    ~Camera();

    void PanForward(float dir);
    void PanRight(float dir);
    void MouseMovement(float dx, float dy);
    void UpdateCameraVectors();

    // Set in constructor
    DirectX::XMFLOAT3 cameraPos = { 0.0f, 0.0f, 0.0f };  // Further right, higher, and further back
    DirectX::XMFLOAT3 cameraForward = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 cameraRight = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 cameraUp = { 0.0f, 0.0f, 0.0f };

    float yaw = 0.0f;    // Rotation around Y axis
    float pitch = 0.0f;  // Rotation around X axis
    float moveSpeed = 0.5f;

    float mouseSensitivity = 0.05f;

    // Collision detection
    //void SetModels(const std::vector<Model*>* models) { this->models = models; }
    bool CheckCollision(const DirectX::XMFLOAT3& newPosition);

    std::vector<Model*>* models = nullptr;
    float collisionRadius = 2.0f;

    std::vector<Model*> collectedDiamonds;
};

