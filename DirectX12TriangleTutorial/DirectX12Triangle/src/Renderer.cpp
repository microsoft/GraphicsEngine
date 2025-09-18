#include "Renderer.h"
//#include "ShaderCompiler.h"
#include <stdexcept>
#include <d3dcompiler.h>

Renderer::Renderer(HWND hwnd, int width, int height)
	: hwnd(hwnd), width(width), height(height) {
    this->model = nullptr;
}

Renderer::~Renderer() {}

void Renderer::Init() {
    InitD3D();
    CreatePipeline();
    CreateAssets();
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
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
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
    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter.Descriptor.ShaderRegister = 0;  // register b0
    rootParameter.Descriptor.RegisterSpace = 0;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 1;
    root_signature_desc.pParameters = &rootParameter;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = nullptr;
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
}

void Renderer::CreatePipeline() {
    // TODO: compile shaders, create root signature, PSO
    // 
    //compile shaders
    HRESULT hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShader, nullptr);
    hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, nullptr);

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

    D3D12_INPUT_ELEMENT_DESC input_elements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    pso_desc.InputLayout.pInputElementDescs = input_elements;
    pso_desc.InputLayout.NumElements = _countof(input_elements);

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
	std::vector<Vertex> model_vertices;
	model->GetVertices(model_vertices);
    void* vertex_mapped_data = nullptr;
    vertex_buffer_upload->Map(0, nullptr, &vertex_mapped_data);
    memcpy(vertex_mapped_data, model_vertices.data(), sizeof(Vertex) * model_vertices.size());
    vertex_buffer_upload->Unmap(0, nullptr);

    std::vector<unsigned int> model_indices;
    model->GetIndices(model_indices);
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
}

void Renderer::Update() {
    // TODO: update MVP constant buffer
    
    triangle_angle++;
    
    // model matrix 
    float angle = DirectX::XMConvertToRadians(triangle_angle);
    DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixRotationY(angle);

    // View matrix 
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 5.0f, -5.0f, 1.0f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, at, up);

    // Projection matrix
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(60.0f), aspect, 0.5f, 100.0f);

    DirectX::XMMATRIX mvp = modelMatrix * view * proj;
    cbData.mvp = XMMatrixTranspose(mvp); // transpose for HLSL row-major
    *mappedCB = cbData;
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

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = constantBuffer->GetGPUVirtualAddress();
    commandList->SetGraphicsRootConstantBufferView(0, cbAddress);

    commandList->DrawIndexedInstanced(model->GetNumIndices(), 1, 0, 0, 0);

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