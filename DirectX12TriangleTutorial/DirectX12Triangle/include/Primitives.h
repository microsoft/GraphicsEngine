#pragma once

struct alignas(256) MVPConstants {
	DirectX::XMMATRIX mvp;
};

struct Vertex {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
};
