#pragma once
#include <DirectXMath.h>

struct alignas(256) MVPConstants {
	DirectX::XMMATRIX mvp;
	DirectX::XMMATRIX model;
	DirectX::XMMATRIX normalMatrix;
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
