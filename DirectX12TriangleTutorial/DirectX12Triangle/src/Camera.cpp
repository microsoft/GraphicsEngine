#include "Camera.h"
#include <algorithm>

Camera::Camera()
{
    // Set initial camera position
    cameraPos = { 20.0f, 10.0f, -60.0f };
    cameraForward = { 0.0f, 0.0f, 1.0f };
    cameraUp = { 0.0f, 1.0f, 0.0f };
    cameraRight = { 1.0f, 0.0f, 0.0f };
    
    yaw = 0.0f;
    pitch = 0.0f;
    mouseSensitivity = 0.005f;
    
    UpdateCameraVectors();
}

Camera::~Camera()
{
}

bool Camera::CheckCollision(const DirectX::XMFLOAT3& newPosition) {
    if (!models) return false;

    BoundingBox cameraBounds;
    const float walkingHeight = newPosition.y; // already constrained externally
    float yMin = walkingHeight - collisionRadius;
    float yMax = walkingHeight + collisionRadius;

    cameraBounds.SetBbox(
        newPosition.x - collisionRadius,
        newPosition.x + collisionRadius,
        newPosition.z - collisionRadius,
        newPosition.z + collisionRadius,
        yMin,
        yMax
    );

    for (const auto& model : *models) {
        if (!model) continue;
        if (cameraBounds.Intersects(cameraBounds, model->b)) {
            return true;
        }
    }
    return false;
}

void Camera::PanForward(float dir)
{
    //cameraPos.x += sin(yaw) * dir;
    //cameraPos.z += cos(yaw) * dir;

    // Keep camera at fixed walking height
    const float walkingHeight = 5.0f;
    //cameraPos.y = walkingHeight;

    DirectX::XMFLOAT3 newPos;
    newPos.x = cameraPos.x + sin(yaw) * dir;
    newPos.y = walkingHeight;
    newPos.z = cameraPos.z + cos(yaw) * dir;

    if (!CheckCollision(newPos)) {
        cameraPos = newPos;
    }
}

void Camera::PanRight(float dir)
{
    //cameraPos.x += sin(yaw + DirectX::XM_PIDIV2) * dir;
    //cameraPos.z += cos(yaw + DirectX::XM_PIDIV2) * dir;

    // Keep camera at fixed walking height
    const float walkingHeight = 5.0f;
    //cameraPos.y = walkingHeight;

    DirectX::XMFLOAT3 newPos;
    newPos.x = cameraPos.x + sin(yaw + DirectX::XM_PIDIV2) * dir;
    newPos.y = walkingHeight;
    newPos.z = cameraPos.z + cos(yaw + DirectX::XM_PIDIV2) * dir;

    if (!CheckCollision(newPos)) {
        cameraPos = newPos;
    }
}

void Camera::MouseMovement(float dx, float dy)
{
	yaw += dx * mouseSensitivity;
	pitch -= dy * mouseSensitivity;

	// clamping due to gimbal 
	const float maxPitch = DirectX::XM_PIDIV2 - 0.1f; // ~89 degrees
	pitch = std::max(-maxPitch, std::min(maxPitch, pitch));

	UpdateCameraVectors();
}

void Camera::UpdateCameraVectors()
{
    // Calculate new forward vector based on yaw and pitch
    cameraForward.x = sin(yaw) * cos(pitch);
    cameraForward.y = sin(pitch);
    cameraForward.z = cos(yaw) * cos(pitch);

    // Normalize forward vector
    DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&cameraForward);
    forward = DirectX::XMVector3Normalize(forward);
    DirectX::XMStoreFloat3(&cameraForward, forward);

    // Calculate right and up vectors
    DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, worldUp));
    DirectX::XMVECTOR up = DirectX::XMVector3Cross(right, forward);

    DirectX::XMStoreFloat3(&cameraRight, right);
    DirectX::XMStoreFloat3(&cameraUp, up);
}
