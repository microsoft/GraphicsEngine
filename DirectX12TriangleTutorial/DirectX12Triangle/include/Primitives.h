#pragma once
#include <DirectXMath.h>

struct MVPConstants {
    DirectX::XMMATRIX mvp;
    DirectX::XMMATRIX model;
    DirectX::XMMATRIX normalMatrix;
    DirectX::XMFLOAT3 viewPos;
    float _padView;  // Rename to match HLSL
};

struct MaterialConstants {
    DirectX::XMFLOAT4 ambient;
    DirectX::XMFLOAT4 diffuse;
    DirectX::XMFLOAT4 specular;
    float shininess;
    DirectX::XMFLOAT3 padding;  // This MUST come after shininess to match HLSL
    
    // Fog parameters
    float fogStart;
    float fogEnd;
    float fogDensity;
    float _padFog;
    
    // Flashlight parameters  
    DirectX::XMFLOAT3 flashlightPos;
    float flashlightRange;
    DirectX::XMFLOAT3 flashlightDir;
    float flashlightAngle;
    float flashlightIntensity;
    DirectX::XMFLOAT3 _padFlash;
};

struct Vertex {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;        // UV should come before normal
	DirectX::XMFLOAT3 normal;
};