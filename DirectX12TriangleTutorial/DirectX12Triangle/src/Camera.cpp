#include "Camera.h"
#include <algorithm>
#include <Audio.h>

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

bool Camera::IsLookingAtModel(Model* model, float threshold) {
    if (!model) return false;

    // Get model position
    DirectX::XMFLOAT3 modelPos = model->GetPosition();

    // Calculate direction from camera to model
    DirectX::XMFLOAT3 toModel;
    toModel.x = modelPos.x - cameraPos.x;
    toModel.y = modelPos.y - cameraPos.y;
    toModel.z = modelPos.z - cameraPos.z;

    // Normalize the direction to model
    DirectX::XMVECTOR toModelVec = DirectX::XMLoadFloat3(&toModel);
    toModelVec = DirectX::XMVector3Normalize(toModelVec);

    // Get camera forward vector
    DirectX::XMVECTOR forwardVec = DirectX::XMLoadFloat3(&cameraForward);

    // Calculate dot product
    DirectX::XMVECTOR dotVec = DirectX::XMVector3Dot(forwardVec, toModelVec);
    float dot = DirectX::XMVectorGetX(dotVec);

    // Check if dot product exceeds threshold (closer to 1.0 means more aligned)
    return dot >= threshold;
}

bool Camera::CheckCollision(const DirectX::XMFLOAT3& newPosition) {
    if (!models) return false;

    // Create a bounding box around the camera position
    BoundingBox cameraBounds;
    cameraBounds.SetBbox(newPosition.x - collisionRadius, 
        newPosition.x + collisionRadius, 
        newPosition.z - collisionRadius, 
        newPosition.z + collisionRadius,
        10.0f,
        10.0f);

	bool isColliding = false;

    // Check collision with each model
    for (const auto& model : *models) {
        if (model) {
            if (model->isRemovable) {
				collectedDiamonds.push_back(model);
                break;
            }
            else {
                isColliding = cameraBounds.Intersects(model->b, cameraBounds);
            }
        }

        if (isColliding) {
            break;
		}
    }

    return isColliding; // No collision
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
