#include "Renderer.h"
//#include "ShaderCompiler.h"
#include <stdexcept>
#include <iostream>  // for debug output
#include "directx/d3dx12.h"
#include "File.h"

Renderer::Renderer(HWND hwnd, int width, int height)
    : hwnd(hwnd), width(width), height(height), 
      fence_event(nullptr), fence_value(0), frameIndex(0),
      rtvDescriptorSize(0), constantBuffer(nullptr), materialBuffer(nullptr),
      mappedCB(nullptr), mappedMat(nullptr) {
    this->models = std::vector<Model*>();
    this->c = Camera();
    
    // Initialize arrays
    renderTargets[0] = nullptr;
    renderTargets[1] = nullptr;
    fenceValues[0] = 0;
    fenceValues[1] = 0;
    
    // Initialize structs
    cbData = {};
    matData = {};
}

Renderer::~Renderer() {}

void Renderer::Init() {
    InitD3D();
    CreatePipeline();
    CreateAssets();
    CreateTextureResources();
    
    // Initialize viewport and scissor rect
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(width);
    scissorRect.bottom = static_cast<LONG>(height);
}

void Renderer::UpdateTextures() {
	std::string flagPath = GetAssetPath("UpdateTexture.txt");
    if (std::filesystem::exists(flagPath)) {
        for (auto& model : models) {
            model->UpdateTextures();
        }
        CreateTextureResources();
		std::filesystem::remove(flagPath);
    }
}

// for transparency (ex)
// how to blend w/ prev draw calls
void Renderer::SetBlendState(D3D12_BLEND_DESC& blend_desc) {
    blend_desc = {};

    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC default_render_target_blend_desc = {};
    default_render_target_blend_desc.BlendEnable = FALSE;
    default_render_target_blend_desc.LogicOpEnable = FALSE;
    default_render_target_blend_desc.SrcBlend = D3D12_BLEND_ONE;
    default_render_target_blend_desc.DestBlend = D3D12_BLEND_ZERO;
    default_render_target_blend_desc.BlendOp = D3D12_BLEND_OP_ADD;
    default_render_target_blend_desc.SrcBlendAlpha = D3D12_BLEND_ONE;
    default_render_target_blend_desc.DestBlendAlpha = D3D12_BLEND_ZERO;
    default_render_target_blend_desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    default_render_target_blend_desc.LogicOp = D3D12_LOGIC_OP_NOOP;
    default_render_target_blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
        blend_desc.RenderTarget[i] = default_render_target_blend_desc;
    }
}

// what should be px
void Renderer::SetRasterizerState(D3D12_RASTERIZER_DESC& rasterizer_desc) {
    rasterizer_desc = {};

    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;  
    rasterizer_desc.FrontCounterClockwise = FALSE;
    rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.MultisampleEnable = FALSE;
    rasterizer_desc.AntialiasedLineEnable = FALSE;
    rasterizer_desc.ForcedSampleCount = 0;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

// depth buff
void Renderer::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC& depth_stencil_desc) {
    depth_stencil_desc = {};

    depth_stencil_desc.DepthEnable = TRUE;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    depth_stencil_desc.StencilEnable = FALSE;
    depth_stencil_desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth_stencil_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

    depth_stencil_desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil_desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil_desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    depth_stencil_desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil_desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil_desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil_desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
}

void Renderer::InitD3D() {
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

    // Command queue
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue));

    // Command allocator = allocating space on GPU for commands 
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    commandAllocator->Reset();

    // Command List = store list fo cmds want to execute (kind of like Draw in OpenGL)
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
    commandList->Close();

    //helper object to create a swap chain
    UINT dxgiFactoryFlags = 0; // this is for debug help
    ComPtr<IDXGIFactory4> factory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    // Swapchain
    // basically this is ping-ponging what's being rendered 
    // 1 being shown while 2 being drawn then swap & vice versa
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    hr = factory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);

    //cast the swap chain to IDXGISwapChain3 to leverage the latest features
    swapChain = {};
    tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
    tempSwapChain->Release();
    tempSwapChain = nullptr;

    // RTV heap
    //memory descriptor heap to store render target views(RTV). Descriptor describes how to interperate resource memory.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < 2; i++) {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }

    CreateDepthBuffer(width, height);
    CreateFenceObjects();
    CreateRootSignature();
    CreateConstBuffer();
}

void Renderer::CreateDepthBuffer(int width, int height)
{
    //memory descriptor heap to store render target views(DSV). Descriptor describes how to interperate resource memory.
    // this is for depth buffer
    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
    dsv_heap_desc.NumDescriptors = 1;
    dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap));

    UINT dsv_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    {
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Alignment = 0;
        resource_desc.Width = width;
        resource_desc.Height = height;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_D32_FLOAT;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clear_value = {};
        clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.0f;
        clear_value.DepthStencil.Stencil = 0;

        HRESULT hr = device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clear_value,
            IID_PPV_ARGS(&depth_stencil_buffer));

        D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_heap->GetCPUDescriptorHandleForHeapStart());
        device->CreateDepthStencilView(depth_stencil_buffer, nullptr, dsv_handle);
    }
}

//fence is used to synchronize the CPU with the GPU, so they don't touch the same memory at the same time
void Renderer::CreateFenceObjects()
{
    UINT64 fence_value = 0;
    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

// essentially additional inputs to our shader
// mvp pased in via this for time being -- hardcoded (150&151)
void Renderer::CreateRootSignature()
{
    D3D12_DESCRIPTOR_RANGE descriptorRange = {};
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.NumDescriptors = 1;
    descriptorRange.BaseShaderRegister = 0; // t0
    descriptorRange.RegisterSpace = 0;
    descriptorRange.OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER rootParameters[3] = {};
    
    // MVP constant buffer
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    
    // Material constant buffer
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 1;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    // Texture
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Add sampler
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 3;
    root_signature_desc.pParameters = rootParameters;
    root_signature_desc.NumStaticSamplers = 1;
    root_signature_desc.pStaticSamplers = &sampler;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* signature_blob = nullptr;
    ID3DBlob* error_blob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob);
    hr = device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

    if (signature_blob) {
        signature_blob->Release();
        signature_blob = nullptr;
    }
    if (error_blob) {
        error_blob->Release();
        error_blob = nullptr;
    }
}

// this is for passing the MVP 
// gotta alloc space & bind
void Renderer::CreateConstBuffer()
{
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = (sizeof(MVPConstants) + 255) & ~255; // must be 256-byte aligned
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantBuffer)
    );

    // Map once, keep pointer around
    hr = constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedCB));

    // Material Buffer
    bufferDesc.Width = (sizeof(MaterialConstants) + 255) & ~255;
    hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&materialBuffer)
    );
    hr = materialBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMat));
    
    // Set default material values
    matData.ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    matData.diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    matData.specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    matData.shininess = 32.0f;
    *mappedMat = matData;
}

void Renderer::CreatePipeline() {
    HRESULT hr;
    ID3DBlob* vsErrors = nullptr;
    ID3DBlob* psErrors = nullptr;
    hr = D3DCompileFromFile(L"shaders\\shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vertexShader, &vsErrors);
    if (FAILED(hr)) {
        if (vsErrors) {
            OutputDebugStringA((char*)vsErrors->GetBufferPointer());
            vsErrors->Release();
        }
        throw std::runtime_error("Failed to compile vertex shader (shaders/shader.hlsl)");
    }
    hr = D3DCompileFromFile(L"shaders\\shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG, 0, &pixelShader, &psErrors);
    if (FAILED(hr)) {
        if (psErrors) {
            OutputDebugStringA((char*)psErrors->GetBufferPointer());
            psErrors->Release();
        }
        throw std::runtime_error("Failed to compile pixel shader (shaders/shader.hlsl)");
    }

    // Pipeline state -> basically everything we need is attached to this
    // shaders, blend mode, depth buff, params, primitives, etc.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = rootSignature;
    // sets up the shaders
    pso_desc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    pso_desc.VS.BytecodeLength = vertexShader->GetBufferSize();
    pso_desc.PS.pShaderBytecode = pixelShader->GetBufferPointer();
    pso_desc.PS.BytecodeLength = pixelShader->GetBufferSize();
    SetBlendState(pso_desc.BlendState);
    pso_desc.SampleMask = UINT_MAX;
    SetRasterizerState(pso_desc.RasterizerState);
    SetDepthStencilState(pso_desc.DepthStencilState);

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    pso_desc.InputLayout.pInputElementDescs = inputElementDescs;
    pso_desc.InputLayout.NumElements = _countof(inputElementDescs);

    pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pso_desc.SampleDesc.Count = 1;
    pso_desc.SampleDesc.Quality = 0;

    device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipelineState));

    vertexShader->Release();
    vertexShader = nullptr;
    pixelShader->Release();
    pixelShader = nullptr;
}

void Renderer::CreateAssets() {
    HRESULT hr;
    if (models.empty()) {
        throw std::runtime_error("No models bound to renderer");
    }

    // Resize buffer vectors to match number of models
    vertex_buffers.resize(models.size());
    vertex_buffers_upload.resize(models.size());
    index_buffers.resize(models.size());
    index_buffers_upload.resize(models.size());

    // Create buffers for each model
    for (size_t i = 0; i < models.size(); ++i) {
        if (models[i] == nullptr) continue;
        
        Model* currentModel = models[i];
        
        // Heap properties
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;

        D3D12_HEAP_PROPERTIES heap_properties_upload = heap_properties;
        heap_properties_upload.Type = D3D12_HEAP_TYPE_UPLOAD;

        // Vertex buffer
        D3D12_RESOURCE_DESC vertex_desc = {};
        vertex_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertex_desc.Alignment = 0;
        vertex_desc.Width = sizeof(Vertex) * currentModel->GetNumVertices();
        vertex_desc.Height = 1;
        vertex_desc.DepthOrArraySize = 1;
        vertex_desc.MipLevels = 1;
        vertex_desc.Format = DXGI_FORMAT_UNKNOWN;
        vertex_desc.SampleDesc.Count = 1;
        vertex_desc.SampleDesc.Quality = 0;
        vertex_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vertex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        hr = device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &vertex_desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&vertex_buffers[i])
        );

        hr = device->CreateCommittedResource(
            &heap_properties_upload,
            D3D12_HEAP_FLAG_NONE,
            &vertex_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertex_buffers_upload[i])
        );

        // Index buffer
        D3D12_RESOURCE_DESC index_desc = {};
        index_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        index_desc.Alignment = 0;
        index_desc.Width = sizeof(unsigned int) * currentModel->GetNumIndices();
        index_desc.Height = 1;
        index_desc.DepthOrArraySize = 1;
        index_desc.MipLevels = 1;
        index_desc.Format = DXGI_FORMAT_UNKNOWN;
        index_desc.SampleDesc.Count = 1;
        index_desc.SampleDesc.Quality = 0;
        index_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        index_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        hr = device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &index_desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&index_buffers[i])
        );

        hr = device->CreateCommittedResource(
            &heap_properties_upload,
            D3D12_HEAP_FLAG_NONE,
            &index_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&index_buffers_upload[i])
        );

        // Copy data to upload buffers
        const std::vector<Vertex>& model_vertices = currentModel->GetVertices();
        const std::vector<unsigned int>& model_indices = currentModel->GetIndices();
        
        void* vertex_mapped_data = nullptr;
        vertex_buffers_upload[i]->Map(0, nullptr, &vertex_mapped_data);
        memcpy(vertex_mapped_data, model_vertices.data(), sizeof(Vertex) * model_vertices.size());
        vertex_buffers_upload[i]->Unmap(0, nullptr);

        void* index_mapped_data = nullptr;
        index_buffers_upload[i]->Map(0, nullptr, &index_mapped_data);
        memcpy(index_mapped_data, model_indices.data(), sizeof(unsigned int) * model_indices.size());
        index_buffers_upload[i]->Unmap(0, nullptr);
    }
    
    // Upload all buffers in one command list execution
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    for (size_t i = 0; i < models.size(); ++i) {
        if (models[i] == nullptr) continue;
        
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.pResource = vertex_buffers[i];
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].Transition.pResource = index_buffers[i];
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(2, barriers);
        
        commandList->CopyResource(vertex_buffers[i], vertex_buffers_upload[i]);
        commandList->CopyResource(index_buffers[i], index_buffers_upload[i]);

        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

        commandList->ResourceBarrier(2, barriers);
    }

    commandList->Close();
    ID3D12CommandList* command_lists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, command_lists);

    // Wait for GPU to finish
    const UINT64 current_fence_value = ++fence_value;
    commandQueue->Signal(fence, current_fence_value);
    if (fence->GetCompletedValue() < current_fence_value) {
        fence->SetEventOnCompletion(current_fence_value, fence_event);
        WaitForSingleObject(fence_event, INFINITE);
    }

    // Build draw ranges for first model only (if using materials)
    // You may want to extend this for per-model materials later
    if (!models.empty() && models[0] != nullptr) {
        drawRanges.clear();
        const auto& materialsRef = models[0]->GetMaterials();
        const auto& faceMaterialIndices = models[0]->GetFaceMaterialIndices();
        size_t totalIndices = models[0]->GetNumIndices();
        
        if (!faceMaterialIndices.empty() && totalIndices % 3 == 0 && !materialsRef.empty()) {
            DrawRange current {0, 0, faceMaterialIndices[0]};
            
            for (UINT tri = 0; tri < totalIndices/3; ++tri) {
                UINT base = tri * 3;
                UINT faceMaterial = (tri < faceMaterialIndices.size()) ? faceMaterialIndices[tri] : 0;
                
                if (faceMaterial != current.materialIndex) {
                    if (current.indexCount > 0) {
                        drawRanges.push_back(current);
                    }
                    current.startIndex = base;
                    current.indexCount = 0;
                    current.materialIndex = faceMaterial;
                }
                current.indexCount += 3;
            }
            if (current.indexCount > 0) drawRanges.push_back(current);
        } else {
            DrawRange dr {0, static_cast<UINT>(totalIndices), 0};
            drawRanges.push_back(dr);
        }
    }
}

void Renderer::HandleForward(float dir)
{
    c.PanForward(dir);
}

void Renderer::HandleX(float dir)
{
    c.PanRight(dir);
}

void Renderer::HandleMouseMove(float deltaX, float deltaY)
{
    c.MouseMovement(deltaX, deltaY);
}

void Renderer::Update() {
    // Update happens per frame, not per model
	UpdateTextures();
}

void Renderer::Render() {
    UINT frameIndex = swapChain->GetCurrentBackBufferIndex();
    
    // Reset command allocator and list
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, pipelineState);
    
    // Set necessary state
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    
    // Set descriptor heaps
    ID3D12DescriptorHeap* ppHeaps[] = { srvHeap };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    
    // Transition render target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets[frameIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);
    
    // Set render target
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += frameIndex * rtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv_heap->GetCPUDescriptorHandleForHeapStart();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    
    // Clear
    float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    
    // Set common pipeline state
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Calculate view and projection matrices (once for all models)
    DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&c.cameraPos);
    DirectX::XMVECTOR camForward = DirectX::XMLoadFloat3(&c.cameraForward);
    DirectX::XMVECTOR camTarget = DirectX::XMVectorAdd(camPos, camForward);
    DirectX::XMVECTOR camUp = DirectX::XMLoadFloat3(&c.cameraUp);
    
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(camPos, camTarget, camUp);
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(45.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f, 1000.0f
    );
    
    // Render each model
    for (size_t i = 0; i < models.size(); ++i) {
        if (models[i] == nullptr) continue;
        
        // Get model's transformation matrix
        DirectX::XMMATRIX model = models[i]->GetModelMatrix();

        // Debug: Print transformation matrix to verify it's not identity
        DirectX::XMFLOAT4X4 modelFloat;
        DirectX::XMStoreFloat4x4(&modelFloat, model);
        OutputDebugStringA(("Model " + std::to_string(i) + " matrix:\n").c_str());
        for (int row = 0; row < 4; row++) {
            std::string rowStr = "";
            for (int col = 0; col < 4; col++) {
                rowStr += std::to_string(modelFloat.m[row][col]) + " ";
            }
            OutputDebugStringA((rowStr + "\n").c_str());
        }
        
        // Update constant buffer for this model
        cbData.mvp = DirectX::XMMatrixTranspose(model * view * proj);
        cbData.model = DirectX::XMMatrixTranspose(model);
        cbData.viewPos = c.cameraPos;
        
        DirectX::XMVECTOR det;
        DirectX::XMMATRIX invModel = DirectX::XMMatrixInverse(&det, model);
        cbData.normalMatrix = DirectX::XMMatrixTranspose(invModel);
        
        *mappedCB = cbData;
        
        // Set vertex and index buffers for this model
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        vertexBufferView.BufferLocation = vertex_buffers[i]->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = sizeof(Vertex) * models[i]->GetNumVertices();
        
        D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
        indexBufferView.BufferLocation = index_buffers[i]->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        indexBufferView.SizeInBytes = sizeof(unsigned int) * models[i]->GetNumIndices();
        
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->IASetIndexBuffer(&indexBufferView);
        
        // Set constant buffers
        commandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, materialBuffer->GetGPUVirtualAddress());
        
        // Set texture for this model
        if (i < modelMaterialRanges.size()) {
            UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
            textureHandle.ptr += modelMaterialRanges[i].startIndex * descriptorSize;
            commandList->SetGraphicsRootDescriptorTable(2, textureHandle);
        }
        
        // Draw
        commandList->DrawIndexedInstanced(models[i]->GetNumIndices(), 1, 0, 0, 0);
    }
    
    // Transition back to present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);
    
    // Execute
    commandList->Close();
    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, commandLists);
    
    swapChain->Present(1, 0);
    
    // Wait for frame to complete
    const UINT64 fenceValue = fenceValues[frameIndex];
    commandQueue->Signal(fence, fenceValue);
    fenceValues[frameIndex]++;
    
    if (fence->GetCompletedValue() < fenceValue) {
        fence->SetEventOnCompletion(fenceValue, fence_event);
        WaitForSingleObject(fence_event, INFINITE);
    }
}
void Renderer::BindModels(const std::vector<Model*>& modelList) {
    this->models = modelList;
}

void Renderer::CreateTextureResources() {
    // Count total materials across all models
    UINT totalMaterialCount = 0;
    for (const auto& model : models) {
        if (model) {
            totalMaterialCount += static_cast<UINT>(model->GetMaterials().size());
            if (model->GetMaterials().empty()) {
                totalMaterialCount++; // Add one for default texture
            }
        }
    }
    
    // Ensure at least one descriptor for default texture
    if (totalMaterialCount == 0) totalMaterialCount = 1;
    
    // Track material ranges for each model
    modelMaterialRanges.clear();
    UINT currentStartIndex = 0;
    for (const auto& model : models) {
        ModelMaterialRange range;
        range.startIndex = currentStartIndex;
        if (model) {
            range.count = static_cast<UINT>(model->GetMaterials().size());
            if (range.count == 0) range.count = 1; // At least one default
        } else {
            range.count = 1;
        }
        modelMaterialRanges.push_back(range);
        currentStartIndex += range.count;
    }
    
    // Create descriptor heap for all textures
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = totalMaterialCount;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));

    materialTextures.resize(totalMaterialCount);
    materialUploadHeaps.resize(totalMaterialCount);
    
    // Create default white texture
    UINT32 whitePixel = 0xFFFFFFFF;
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = 1;
    textureDesc.Height = 1;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultTexture));
    
    // Create upload buffer for white pixel
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = 256; // Minimum 256 byte alignment
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&defaultUploadHeap));

    // Copy white pixel to upload buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    defaultUploadHeap->Map(0, &readRange, &mappedData);
    memcpy(mappedData, &whitePixel, sizeof(UINT32));
    defaultUploadHeap->Unmap(0, nullptr);

    // Upload default texture
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = defaultUploadHeap.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint.Offset = 0;
    srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcLocation.PlacedFootprint.Footprint.Width = 1;
    srcLocation.PlacedFootprint.Footprint.Height = 1;
    srcLocation.PlacedFootprint.Footprint.Depth = 1;
    srcLocation.PlacedFootprint.Footprint.RowPitch = 256;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = defaultTexture.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = defaultTexture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, ppCommandLists);

    const UINT64 fence_value_for_signal = ++fence_value;
    commandQueue->Signal(fence, fence_value_for_signal);
    if (fence->GetCompletedValue() < fence_value_for_signal) {
        fence->SetEventOnCompletion(fence_value_for_signal, fence_event);
        WaitForSingleObject(fence_event, INFINITE);
    }

    // Now create SRVs for each material
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE currentHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    
    UINT globalMaterialIndex = 0;

    // Reset command list for texture uploads
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);
    
    for (size_t modelIdx = 0; modelIdx < models.size(); ++modelIdx) {
        if (!models[modelIdx]) {
            // Null model - just use default texture
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(defaultTexture.Get(), &srvDesc, currentHandle);
            currentHandle.ptr += descriptorSize;
            globalMaterialIndex++;
            continue;
        }
        
        const auto& materials = models[modelIdx]->GetMaterials();
        
        if (materials.empty()) {
            // Model has no materials - create one default SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(defaultTexture.Get(), &srvDesc, currentHandle);
            currentHandle.ptr += descriptorSize;
            globalMaterialIndex++;
        } else {
            // Process each material for this model
            for (size_t matIdx = 0; matIdx < materials.size(); ++matIdx) {
                const Material& mat = materials[matIdx];
                
                // Check if material has a diffuse texture
                if (!mat.diffuseMap.empty() && mat.textureImage.GetWidth() > 0) {
                    // Load actual texture from material
                    int texWidth = mat.textureImage.GetWidth();
                    int texHeight = mat.textureImage.GetHeight();
                    const unsigned char* imageData = mat.textureImage.data();
                    
                    if (imageData) {
                        // Create texture resource
                        D3D12_RESOURCE_DESC textureDesc = {};
                        textureDesc.MipLevels = 1;
                        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        textureDesc.Width = texWidth;
                        textureDesc.Height = texHeight;
                        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
                        textureDesc.DepthOrArraySize = 1;
                        textureDesc.SampleDesc.Count = 1;
                        textureDesc.SampleDesc.Quality = 0;
                        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                        
                        D3D12_HEAP_PROPERTIES heapProps = {};
                        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
                        
                        ComPtr<ID3D12Resource> texture;
                        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
                            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
                        
                        // Create upload buffer
                        UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
                        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
                        
                        D3D12_RESOURCE_DESC bufferDesc = {};
                        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                        bufferDesc.Width = uploadBufferSize;
                        bufferDesc.Height = 1;
                        bufferDesc.DepthOrArraySize = 1;
                        bufferDesc.MipLevels = 1;
                        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
                        bufferDesc.SampleDesc.Count = 1;
                        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
                        
                        ComPtr<ID3D12Resource> uploadBuffer;
                        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
                        
                        // Copy image data to upload buffer
                        D3D12_SUBRESOURCE_DATA subresourceData = {};
                        subresourceData.pData = imageData;
                        subresourceData.RowPitch = texWidth * 4; // 4 bytes per pixel (RGBA)
                        subresourceData.SlicePitch = subresourceData.RowPitch * texHeight;
                        
                        UpdateSubresources(commandList, texture.Get(), uploadBuffer.Get(), 0, 0, 1, &subresourceData);
                        
                        // Transition to shader resource
                        D3D12_RESOURCE_BARRIER barrier = {};
                        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                        barrier.Transition.pResource = texture.Get();
                        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                        commandList->ResourceBarrier(1, &barrier);
                        
                        // Store the texture
                        materialTextures[globalMaterialIndex] = texture;
                        materialUploadHeaps[globalMaterialIndex] = uploadBuffer;
                        
                        // Create SRV for this texture
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Texture2D.MipLevels = 1;
                        device->CreateShaderResourceView(texture.Get(), &srvDesc, currentHandle);
                    } else {
                        // Use default white texture
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Texture2D.MipLevels = 1;
                        device->CreateShaderResourceView(defaultTexture.Get(), &srvDesc, currentHandle);
                    }
                } else {
                    // Use default white texture
                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MipLevels = 1;
                    device->CreateShaderResourceView(defaultTexture.Get(), &srvDesc, currentHandle);
                }
                
                currentHandle.ptr += descriptorSize;
                globalMaterialIndex++;
            }
        }
    }
    // Execute all texture uploads at once
    commandList->Close();
    ID3D12CommandList* ppTextureCommandLists[] = { commandList };  // Use different name
    commandQueue->ExecuteCommandLists(1, ppTextureCommandLists);
    
    // Wait for uploads to complete
    const UINT64 fence_value_for_textures = ++fence_value;
    commandQueue->Signal(fence, fence_value_for_textures);
    if (fence->GetCompletedValue() < fence_value_for_textures) {
        fence->SetEventOnCompletion(fence_value_for_textures, fence_event);
        WaitForSingleObject(fence_event, INFINITE);
    }

}
