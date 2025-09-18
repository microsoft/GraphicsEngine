#include "Renderer.h"
//#include "ShaderCompiler.h"
#include <stdexcept>
#include <d3dcompiler.h>
#include <iostream>  // for debug output

Renderer::Renderer(HWND hwnd, int width, int height)
	: hwnd(hwnd), width(width), height(height) {
    this->model = nullptr;
}

Renderer::~Renderer() {}

void Renderer::Init() {
    InitD3D();
    CreatePipeline();
    CreateAssets();
    CreateTextureResources();
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
    // TODO: upload to GPU
    if (model == nullptr) {
		throw std::runtime_error("No model bound to renderer");
    }

    // allocating space for buffers for model
    //heap properties
    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 1;
    heap_properties.VisibleNodeMask = 1;

    D3D12_HEAP_PROPERTIES heap_properties_upload = heap_properties;
    heap_properties_upload.Type = D3D12_HEAP_TYPE_UPLOAD;

    //resource description
    D3D12_RESOURCE_DESC resource_desc = {};
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = sizeof(Vertex) * model->GetNumVertices();
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    //vertex
    hr = device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&vertex_buffer)
    );

    hr = device->CreateCommittedResource(
        &heap_properties_upload,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertex_buffer_upload)
    );

    //index

    resource_desc.Width = sizeof(unsigned int) * model->GetNumIndices();
    hr = device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&index_buffer)
    );

    hr = device->CreateCommittedResource(
        &heap_properties_upload,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&index_buffer_upload)
    );

    //copy data from CPU to the upload buffers
	const std::vector<Vertex>& model_vertices = model->GetVertices();
    const std::vector<unsigned int>& model_indices = model->GetIndices();
    void* vertex_mapped_data = nullptr;
    vertex_buffer_upload->Map(0, nullptr, &vertex_mapped_data);
    memcpy(vertex_mapped_data, model_vertices.data(), sizeof(Vertex) * model_vertices.size());
    vertex_buffer_upload->Unmap(0, nullptr);

    void* index_mapped_data = nullptr;
    index_buffer_upload->Map(0, nullptr, &index_mapped_data);
    memcpy(index_mapped_data, model_indices.data(), sizeof(unsigned int) * model_indices.size());
    index_buffer_upload->Unmap(0, nullptr);
    
    //Record commands to copy the data from the upload buffer to the fast default buffer
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    D3D12_RESOURCE_BARRIER barrier[2] = {};
    barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier[0].Transition.pResource = vertex_buffer;
    barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier[1].Transition.pResource = index_buffer;
    barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(2, barrier);

    //copy the data from upload to the fast default buffer
    commandList->CopyResource(vertex_buffer, vertex_buffer_upload);
    commandList->CopyResource(index_buffer, index_buffer_upload);

    barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    commandList->ResourceBarrier(2, barrier);

    commandList->Close();

    ID3D12CommandList* command_lists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, command_lists);

    // Wait on the CPU for the GPU frame to finish
    const UINT64 current_fence_value = ++fence_value;
    commandQueue->Signal(fence, current_fence_value);

    if (fence->GetCompletedValue() < current_fence_value) {
        fence->SetEventOnCompletion(current_fence_value, fence_event);
        WaitForSingleObject(fence_event, INFINITE);
    }

    // Build draw ranges grouped by material (triangle = 3 indices)
    drawRanges.clear();
    const auto& materialsRef = model->GetMaterials();
    const auto& indices = model->GetIndices();
    const auto& faceMaterialIndices = model->GetFaceMaterialIndices();
    size_t totalIndices = model->GetNumIndices();
    
    if (!faceMaterialIndices.empty() && totalIndices % 3 == 0 && !materialsRef.empty()) {
        // Build ranges based on consecutive triangles with same material
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
        std::cout << "Using single draw range with material 0" << std::endl;
    }
}

void Renderer::Update() {
    triangle_angle++;
    DirectX::XMMATRIX model = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(static_cast<float>(triangle_angle)));
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(0.0f, 5.0f, -50.0f, 1.0f),
        DirectX::XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f),
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(45.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        100.0f
    );

    cbData.mvp = DirectX::XMMatrixTranspose(model * view * proj);
    cbData.model = DirectX::XMMatrixTranspose(model);
    
    // Calculate normal matrix (transpose of inverse model)
    DirectX::XMVECTOR det;
    DirectX::XMMATRIX invModel = DirectX::XMMatrixInverse(&det, model);
    cbData.normalMatrix = invModel; // Don't transpose since we're already transposing in shader
    
    *mappedCB = cbData;

    // Update material if model has materials
    if (!this->model->GetMaterials().empty()) {
        const Material& mat = this->model->GetMaterials()[0];
        matData.ambient = DirectX::XMFLOAT4(mat.ambient.x, mat.ambient.y, mat.ambient.z, 1.0f);
        matData.diffuse = DirectX::XMFLOAT4(mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, 1.0f);
        matData.specular = DirectX::XMFLOAT4(mat.specular.x, mat.specular.y, mat.specular.z, 1.0f);
        matData.shininess = mat.shininess;
        
        // Debug output
        //std::cout << "Material loaded - Diffuse: " 
        //          << mat.diffuse.x << ", " << mat.diffuse.y << ", " << mat.diffuse.z << std::endl;
    } else {
        // Default materials if none loaded
        matData.ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
        matData.diffuse = DirectX::XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f); // Red for debugging
        matData.specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        matData.shininess = 32.0f;
        
        //std::cout << "Using default red material" << std::endl;
    }
    *mappedMat = matData;
}

void Renderer::Render() {
    // TODO: record command list, clear RT, draw cube
    // Record commands to draw a triangle
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    UINT back_buffer_index = swapChain->GetCurrentBackBufferIndex();

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = renderTargets[back_buffer_index];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv_handle.ptr += back_buffer_index * rtvDescriptorSize;

    // Clear the render target
    float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_heap->GetCPUDescriptorHandleForHeapStart();
    commandList->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set viewport and scissor
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    D3D12_RECT scissor_rect = { 0, 0, width, height };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor_rect);

    commandList->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPipelineState(pipelineState);

    // Draw the triangle
    //command_list->SetGraphicsRoot32BitConstant(0, triangle_angle, 0);

    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
    vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
    vertex_buffer_view.StrideInBytes = sizeof(Vertex);
    vertex_buffer_view.SizeInBytes = sizeof(Vertex) * model->GetNumVertices();
    commandList->IASetVertexBuffers(0, 1, &vertex_buffer_view);

    D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
    index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
    index_buffer_view.SizeInBytes = sizeof(unsigned int) * model->GetNumIndices();
    index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
    commandList->IASetIndexBuffer(&index_buffer_view);

    // Set descriptor heaps - only if we have textures
    if (srvHeap.Get() != nullptr) {
        ID3D12DescriptorHeap* ppHeaps[] = { srvHeap.Get() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    }
    
    // Bind CBVs
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = constantBuffer->GetGPUVirtualAddress();
    commandList->SetGraphicsRootConstantBufferView(0, cbAddress);
    
    D3D12_GPU_VIRTUAL_ADDRESS matAddress = materialBuffer->GetGPUVirtualAddress();
    commandList->SetGraphicsRootConstantBufferView(1, matAddress);
    
    // Multi-material draws
    if (drawRanges.empty()) {
        if (srvHeap.Get() != nullptr) {
            commandList->SetGraphicsRootDescriptorTable(2, srvHeap->GetGPUDescriptorHandleForHeapStart());
        }
        commandList->DrawIndexedInstanced(model->GetNumIndices(), 1, 0, 0, 0);
    } else {
        // Iterate each range; assume one SRV per material laid out sequentially in heap
        UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        for (const auto& dr : drawRanges) {
            if (srvHeap.Get() != nullptr) {
                D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
                gpuHandle.ptr += dr.materialIndex * descriptorSize;
                commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);
            }
            // Update material constants if available
            if (model && dr.materialIndex < model->GetMaterials().size()) {
                const Material& m = model->GetMaterials()[dr.materialIndex];
                matData.ambient = DirectX::XMFLOAT4(m.ambient.x, m.ambient.y, m.ambient.z, 1.0f);
                matData.diffuse = DirectX::XMFLOAT4(m.diffuse.x, m.diffuse.y, m.diffuse.z, 1.0f);
                matData.specular = DirectX::XMFLOAT4(m.specular.x, m.specular.y, m.specular.z, 1.0f);
                matData.shininess = m.shininess;
                *mappedMat = matData;
            }
            commandList->DrawIndexedInstanced(dr.indexCount, 1, dr.startIndex, 0, 0);
        }
    }

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = renderTargets[back_buffer_index];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    commandList->Close();

    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, commandLists);

    swapChain->Present(1, 0);

    // Wait on the CPU for the GPU frame to finish
    const UINT64 current_fence_value = ++fence_value;
    commandQueue->Signal(fence, current_fence_value);

    if (fence->GetCompletedValue() < current_fence_value) {
        fence->SetEventOnCompletion(current_fence_value, fence_event);
        WaitForSingleObject(fence_event, INFINITE);
    }
}

void Renderer::BindModel(Model &model) {
    this->model = &model;
}

void Renderer::CreateTextureResources() {
    // Prepare multi-material descriptor heap
    UINT materialCount = (model) ? static_cast<UINT>(model->GetMaterials().size()) : 0;
    if (materialCount == 0) materialCount = 1; // at least one default
    
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = materialCount;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));

    materialTextures.resize(materialCount);
    materialUploadHeaps.resize(materialCount);
    
    // Create 1x1 white texture
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

    // Now create SRV for each material slot
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE currentHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    
    if (model && !model->GetMaterials().empty()) {
        for (UINT m = 0; m < materialCount; ++m) {
            const Material& mat = model->GetMaterials()[m];
            const unsigned char* imgData = const_cast<Image&>(mat.textureImage).data();
            
            if (!mat.diffuseMap.empty() && imgData != nullptr) {
                // Create actual texture for this material
                UINT width = static_cast<UINT>(mat.textureImage.GetWidth());
                UINT height = static_cast<UINT>(mat.textureImage.GetHeight());

                D3D12_RESOURCE_DESC texDesc = {};
                texDesc.MipLevels = 1;
                texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                texDesc.Width = width;
                texDesc.Height = height;
                texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
                texDesc.DepthOrArraySize = 1;
                texDesc.SampleDesc.Count = 1;
                texDesc.SampleDesc.Quality = 0;
                texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

                D3D12_HEAP_PROPERTIES props = {};
                props.Type = D3D12_HEAP_TYPE_DEFAULT;
                device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &texDesc, 
                    D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&materialTextures[m]));

                // Create upload heap
                UINT64 uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
                UINT64 uploadSize = height * uploadPitch;
                props.Type = D3D12_HEAP_TYPE_UPLOAD;
                
                D3D12_RESOURCE_DESC uploadDesc = {};
                uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                uploadDesc.Width = uploadSize;
                uploadDesc.Height = 1;
                uploadDesc.DepthOrArraySize = 1;
                uploadDesc.MipLevels = 1;
                uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
                uploadDesc.SampleDesc.Count = 1;
                uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
                
                device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &uploadDesc, 
                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialUploadHeaps[m]));
                
                // Copy texture data
                void* mapped = nullptr;
                D3D12_RANGE range = {0,0};
                materialUploadHeaps[m]->Map(0, &range, &mapped);
                unsigned char* destData = static_cast<unsigned char*>(mapped);
                for (UINT y = 0; y < height; ++y) {
                    memcpy(destData + y * uploadPitch, imgData + y * width * 4, width * 4);
                }
                materialUploadHeaps[m]->Unmap(0, nullptr);

                // Upload texture
                commandAllocator->Reset();
                commandList->Reset(commandAllocator, nullptr);
                
                D3D12_TEXTURE_COPY_LOCATION src = {};
                src.pResource = materialUploadHeaps[m].Get();
                src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                src.PlacedFootprint.Offset = 0;
                src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                src.PlacedFootprint.Footprint.Width = width;
                src.PlacedFootprint.Footprint.Height = height;
                src.PlacedFootprint.Footprint.Depth = 1;
                src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(uploadPitch);
                
                D3D12_TEXTURE_COPY_LOCATION dst = {};
                dst.pResource = materialTextures[m].Get();
                dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dst.SubresourceIndex = 0;
                
                commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
                
                D3D12_RESOURCE_BARRIER bar = {};
                bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                bar.Transition.pResource = materialTextures[m].Get();
                bar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                bar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                commandList->ResourceBarrier(1, &bar);
                
                commandList->Close();
                ID3D12CommandList* lists[] = { commandList };
                commandQueue->ExecuteCommandLists(1, lists);
                
                const UINT64 fval = ++fence_value;
                commandQueue->Signal(fence, fval);
                if (fence->GetCompletedValue() < fval) {
                    fence->SetEventOnCompletion(fval, fence_event);
                    WaitForSingleObject(fence_event, INFINITE);
                }

                // Create SRV for this texture
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                device->CreateShaderResourceView(materialTextures[m].Get(), &srvDesc, currentHandle);
                
                std::cout << "Loaded texture (material " << m << "): " << width << "x" << height << " from " << mat.diffuseMap << std::endl;
            } else {
                // Create SRV pointing to default white texture
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                device->CreateShaderResourceView(defaultTexture.Get(), &srvDesc, currentHandle);
                
                std::cout << "Using default white texture for material " << m << std::endl;
            }
            
            // Always advance the handle to maintain alignment
            currentHandle.ptr += descriptorSize;
        }
    } else {
        // Create default texture SRV if no model/materials
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(defaultTexture.Get(), &srvDesc, currentHandle);
    }
}