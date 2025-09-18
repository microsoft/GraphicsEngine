#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include "Model.h"
#include <vector>
#include "Primitives.h"

using Microsoft::WRL::ComPtr;

class Renderer
{
public:
    Renderer(HWND hwnd, int width, int height);
    ~Renderer();

    void Init();
    void Update();
    void Render();

	void BindModel(Model &model);
private:
    void InitD3D();
    void SetBlendState(D3D12_BLEND_DESC& blend_desc);
    void SetRasterizerState(D3D12_RASTERIZER_DESC& rasterizer_desc);
    void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC& depth_stencil_desc);
    void CreateDepthBuffer(int width, int height);
    void CreateFenceObjects();
    void CreateRootSignature();
    void CreateConstBuffer();
    void CreatePipeline();
    void CreateAssets();

    HWND hwnd;
    int width, height;

    IDXGISwapChain3* swapChain = nullptr;
    IDXGISwapChain* tempSwapChain = nullptr;

    ID3D12Device* device = nullptr;
    
    ID3D12DescriptorHeap* rtvHeap = nullptr;
    ID3D12Resource* renderTargets[2];

    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12CommandAllocator* commandAllocator = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;

    ID3D12Fence* fence = nullptr;
    UINT64 fence_value;
    HANDLE fence_event;

    ID3D12DescriptorHeap* dsv_heap = nullptr;
    ID3D12Resource* depth_stencil_buffer = nullptr;

    ID3D12PipelineState* pipelineState = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;

    ID3D12Resource* constantBuffer;
    MVPConstants cbData;       // CPU-side struct
    MVPConstants* mappedCB = nullptr;

    ID3DBlob* vertexShader = nullptr;
    ID3DBlob* pixelShader = nullptr;

    UINT rtvDescriptorSize;
    UINT frameIndex;

    Model* model;

    unsigned int triangle_angle = 10;

    bool running = true;

    // shared mem = both cpu & gpu 
    // want to move to buffs which only on gpu mem (vram?)
    ID3D12Resource* vertex_buffer = nullptr; //fast. GPU access only
    ID3D12Resource* vertex_buffer_upload = nullptr; //slow. CPU and GPU access
    ID3D12Resource* index_buffer = nullptr; //fast. GPU access only
    ID3D12Resource* index_buffer_upload = nullptr; //slow. CPU and GPU access
};

