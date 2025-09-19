#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

class Camera
{
public:
    Camera();
    ~Camera();

    void PanForward(float dir);
    void PanRight(float dir);
    void MouseMovement(float dx, float dy);
    void UpdateCameraVectors();

    DirectX::XMFLOAT3 cameraPos = { 0.0f, 5.0f, -50.0f };
    DirectX::XMFLOAT3 cameraForward = { 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 cameraRight = { 1.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 cameraUp = { 0.0f, 1.0f, 0.0f };

    float yaw = 0.0f;    // Rotation around Y axis
    float pitch = 0.0f;  // Rotation around X axis
    float moveSpeed = 0.5f;

    float mouseSensitivity = 0.1f;
};

