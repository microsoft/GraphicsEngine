#pragma once
#include <DirectXMath.h>

struct MVPConstants {
    DirectX::XMMATRIX mvp;
    DirectX::XMMATRIX model;
    DirectX::XMMATRIX normalMatrix;
    DirectX::XMFLOAT3 viewPos;  // Add this line
    float padding;               // Add padding to ensure 16-byte alignment
};

struct MaterialConstants {
	DirectX::XMFLOAT4 ambient;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	float shininess;
	DirectX::XMFLOAT3 padding; // Ensure 16-byte alignment
};

struct Vertex {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;        // UV should come before normal
	DirectX::XMFLOAT3 normal;
};
