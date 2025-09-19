#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "Model.h"
#include <vector>
#include "Primitives.h"
#include "Camera.h"

using Microsoft::WRL::ComPtr;

class Renderer
{
public:
    Renderer(HWND hwnd, int width, int height);
    ~Renderer();

    void Init();
    void Update();
    void Render();

    void HandleForward(float dir);
    void HandleX(float dir);
    void HandleMouseMove(float deltaX, float deltaY);

    void BindModels(const std::vector<Model*>& models);
    void RenderModel(Model* model, int modelIndex);
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
    void CreateTextureResources();

    Camera c;

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

    ID3D12Resource* materialBuffer;
    MaterialConstants matData;
    MaterialConstants* mappedMat = nullptr;

    ID3DBlob* vertexShader = nullptr;
    ID3DBlob* pixelShader = nullptr;

    UINT rtvDescriptorSize;
    UINT frameIndex;

    std::vector<Model*> models;
    int currentModelIndex = 0;

    unsigned int triangle_angle = 10;

    bool running = true;

    // shared mem = both cpu & gpu 
    // want to move to buffs which only on gpu mem (vram?)
    std::vector<ID3D12Resource*> vertex_buffers;
    std::vector<ID3D12Resource*> vertex_buffers_upload;
    std::vector<ID3D12Resource*> index_buffers;
    std::vector<ID3D12Resource*> index_buffers_upload;

    ComPtr<ID3D12Resource> textureResource;

    // Multi-material support
    struct DrawRange { UINT startIndex; UINT indexCount; UINT materialIndex; };
    std::vector<DrawRange> drawRanges; // Built after uploading indices
    std::vector<ComPtr<ID3D12Resource>> materialTextures; // One per material
    std::vector<ComPtr<ID3D12Resource>> materialUploadHeaps; // Keep alive until copies finish

    ID3D12DescriptorHeap* srvHeap = nullptr;
    ComPtr<ID3D12Resource> defaultTexture;  
    ComPtr<ID3D12Resource> defaultUploadHeap; 

    D3D12_VIEWPORT viewport = {};
    D3D12_RECT scissorRect = {};
    UINT64 fenceValues[2] = {};  // Per frame fence values

    struct ModelMaterialRange {
        UINT startIndex;
        UINT count;
    };
    std::vector<ModelMaterialRange> modelMaterialRanges;
};

