#include "Camera.h"
#include <algorithm>

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::PanForward(float dir)
{
	cameraPos.x += cameraForward.x * dir;
	cameraPos.y += cameraForward.y * dir;
	cameraPos.z += cameraForward.z * dir;
}

void Camera::PanRight(float dir)
{
	cameraPos.x += cameraRight.x * dir;
	cameraPos.y += cameraRight.y * dir;
	cameraPos.z += cameraRight.z * dir;
}

void Camera::MouseMovement(float dx, float dy)
{
	yaw += dx * mouseSensitivity;
	pitch += dy * mouseSensitivity;

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
