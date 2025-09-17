#include <wrl.h>
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <DirectXMath.h>
#include <map>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indicies;
};

struct alignas(256) MVPConstants {
    DirectX::XMMATRIX mvp;
};

Model load_model_from_obj() {

    Model model;
    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;

    model.vertices = {
        // Front face
        { {-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 0
        { {-0.5f, +0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 1
        { {+0.5f, +0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 2
        { {+0.5f, -0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },  // 3

        // Back face
        { {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 4
        { {+0.5f, +0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 5
        { {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 6
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f} }, // 7

        // Left face
        { {-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f} }, // 8
        { {-0.5f, +0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f} }, // 9
        { {-0.5f, +0.5f, +0.5f}, {-1.0f, 0.0f, 0.0f} }, // 10
        { {-0.5f, -0.5f, +0.5f}, {-1.0f, 0.0f, 0.0f} }, // 11

        // Right face
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f, 0.0f} },  // 12
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 0.0f, 0.0f} },  // 13
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },  // 14
        { {+0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },  // 15

        // Top face
        { {-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },  // 16
        { {-0.5f, +0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 17
        { {+0.5f, +0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 18
        { {+0.5f, +0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },  // 19

        // Bottom face
        { {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f} }, // 20
        { {-0.5f, -0.5f, +0.5f}, {0.0f, -1.0f, 0.0f} }, // 21
        { {+0.5f, -0.5f, +0.5f}, {0.0f, -1.0f, 0.0f} }, // 22
        { {+0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f} }  // 23
    };

    model.indicies = {
        // Front face (0,1,2,3)
        0, 1, 2,  0, 2, 3,
        // Back face (4,5,6,7)
        4, 5, 6,  4, 6, 7,
        // Left face (8,9,10,11)
        8, 9, 10, 8, 10, 11,
        // Right face (12,13,14,15)
        12, 13, 14, 12, 14, 15,
        // Top face (16,17,18,19)
        16, 17, 18, 16, 18, 19,
        // Bottom face (20,21,22,23)
        20, 21, 22, 20, 22, 23
    };

    return model;
}

/*
Model load_model_from_obj(const std::string& path) {
    std::ifstream file(path);  // Open the file

    if (!file.is_open()) {
        return {};
    }

    Model model;
    std::vector<DirectX::XMFLOAT3> temp_positions;
    std::vector<DirectX::XMFLOAT3> temp_normals;
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> index_map;
    std::vector<unsigned int> temp_face_indices;

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);

        std::string prefix;
        ss >> prefix;

        if (prefix == "v") { //vertex position
            DirectX::XMFLOAT3 position;
            ss >> position.x >> position.y >> position.z;
            temp_positions.push_back(position);
        }
        else if (prefix == "vn") { //vertex normal
            DirectX::XMFLOAT3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "f") {
            temp_face_indices.clear();

            std::string vertexindex_slashslash_normalindex;
            while (ss >> vertexindex_slashslash_normalindex) {
                size_t slash_pos = vertexindex_slashslash_normalindex.find("//");

                std::string vertex_index_str = vertexindex_slashslash_normalindex.substr(0, slash_pos);
                std::string normal_index_str = vertexindex_slashslash_normalindex.substr(slash_pos + 2);
                unsigned int vertex_index = std::stoi(vertex_index_str) - 1;
                unsigned int normal_index = std::stoi(normal_index_str) - 1;

                // Check if the vertex and normal index pair already exists in the map
                auto it = index_map.find({ vertex_index, normal_index });

                if (it != index_map.end()) {
                    temp_face_indices.push_back(it->second);
                }
                else {
                    // If it doesn't exist, create a new vertex and add it to the model
                    Vertex vertex;
                    vertex.position = temp_positions[vertex_index];
                    vertex.normal = temp_normals[normal_index];
                    unsigned int new_index = static_cast<unsigned int>(model.vertices.size());
                    model.vertices.push_back(vertex);
                    // Store the new index in the map
                    index_map[{ vertex_index, normal_index }] = new_index;
                    temp_face_indices.push_back(new_index);
                }
            }

            for (int i = 0; i < (int)temp_face_indices.size() - 2; i++) {
                model.indicies.push_back(temp_face_indices[0]);
                model.indicies.push_back(temp_face_indices[i + 1]);
                model.indicies.push_back(temp_face_indices[i + 2]);
            }
        }
    }
    file.close();  // Close the file
    return model;
}
*/

// for transparency (ex)
// how to blend w/ prev draw calls
void set_blend_state(D3D12_BLEND_DESC& blend_desc) {
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
void set_rasterizer_state(D3D12_RASTERIZER_DESC& rasterizer_desc) {
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
void set_depth_stencil_state(D3D12_DEPTH_STENCIL_DESC& depth_stencil_desc) {
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

// checks if user closes screen -> if so running = false in winMain
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    const int width = 800;
    const int height = 800;

    // Register a simple window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DirectX12Triangle";
    RegisterClass(&wc);

    // Create a window
    HWND hwnd = CreateWindow(wc.lpszClassName, L"DirectX 12 Triangle", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hwnd, nCmdShow);

    //The device is like a virtual representation of the GPU
    ID3D12Device* device = nullptr;
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

    //command queue decides which order the command lists should execute. In our case, we only have one command list.
    ID3D12CommandQueue* command_queue = nullptr;
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    hr = device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue));

    //command allocator is used to allocate memory on the GPU for commands
    ID3D12CommandAllocator* command_allocator = nullptr;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator));
    hr = command_allocator->Reset();

    //command list is used to store a list of commands that we want to execute on the GPU
    ID3D12GraphicsCommandList* command_list = nullptr;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator, nullptr, IID_PPV_ARGS(&command_list));
    hr = command_list->Close();

    //helper object to create a swap chain
    IDXGIFactory4* factory = nullptr;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    //create swap chain
    // basically this is ping-ponging what's being rendered 
    // 1 being shown while 2 being drawn then swap & vice versa
    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.BufferDesc.Width = width;
    swap_chain_desc.BufferDesc.Height = height;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.OutputWindow = hwnd;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.Windowed = TRUE;

    IDXGISwapChain* temp_swap_chain = nullptr;
    hr = factory->CreateSwapChain(command_queue, &swap_chain_desc, &temp_swap_chain);

    //cast the swap chain to IDXGISwapChain3 to leverage the latest features
    IDXGISwapChain3* swap_chain = {};
    hr = temp_swap_chain->QueryInterface(IID_PPV_ARGS(&swap_chain));
    temp_swap_chain->Release();
    temp_swap_chain = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer;
    MVPConstants cbData;
    MVPConstants* mappedCB = nullptr;

    //memory descriptor heap to store render target views(RTV). Descriptor describes how to interperate resource memory.
    ID3D12DescriptorHeap* rtv_heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.NumDescriptors = 2;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    hr = device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap));

    ID3D12Resource* render_targets[2];

    UINT rtv_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < 2; i++) {
            hr = swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]));

            device->CreateRenderTargetView(render_targets[i], nullptr, rtv_handle);
            rtv_handle.ptr += rtv_increment_size;
        }
    }

    //memory descriptor heap to store render target views(DSV). Descriptor describes how to interperate resource memory.
    ID3D12DescriptorHeap* dsv_heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
    dsv_heap_desc.NumDescriptors = 1;
    dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    hr = device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap));

    ID3D12Resource* depth_stencil_buffer = nullptr;

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

        hr = device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clear_value,
            IID_PPV_ARGS(&depth_stencil_buffer));

        D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_heap->GetCPUDescriptorHandleForHeapStart());
        device->CreateDepthStencilView(depth_stencil_buffer, nullptr, dsv_handle);
    }

    //fence is used to synchronize the CPU with the GPU, so they don't touch the same memory at the same time
    ID3D12Fence* fence = nullptr;
    UINT64 fence_value = 0;
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    HANDLE fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    //Root signature is like have many object buffers and textures we want to use when drawing.
    //For our rotating triangle, we only need a single constant that is going to be our angle
    //D3D12_ROOT_PARAMETER root_parameters[1] = {};
    //root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    //root_parameters[0].Constants.Num32BitValues = 1;
    //root_parameters[0].Constants.ShaderRegister = 0;
    //root_parameters[0].Constants.RegisterSpace = 0;
    //root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter.Descriptor.ShaderRegister = 0;  // register b0
    rootParameter.Descriptor.RegisterSpace = 0;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    ID3D12RootSignature* root_signature = nullptr;
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = 1;
    root_signature_desc.pParameters = &rootParameter;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = nullptr;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* signature_blob = nullptr;
    ID3DBlob* error_blob = nullptr;
    hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob);
    hr = device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature));

    if (signature_blob) {
        signature_blob->Release();
        signature_blob = nullptr;
    }
    if (error_blob) {
        error_blob->Release();
        error_blob = nullptr;
    }

    //compile shaders
    ID3DBlob* vertex_shader = nullptr;
    ID3DBlob* pixel_shader = nullptr;
    hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertex_shader, nullptr);
    hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixel_shader, nullptr);

    // Pipeline state -> basically everything we need is attached to this
    // shaders, blend mode, depth buff, params, primitives, etc.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = root_signature;
    // sets up the shaders
    pso_desc.VS.pShaderBytecode = vertex_shader->GetBufferPointer();
    pso_desc.VS.BytecodeLength = vertex_shader->GetBufferSize();
    pso_desc.PS.pShaderBytecode = pixel_shader->GetBufferPointer();
    pso_desc.PS.BytecodeLength = pixel_shader->GetBufferSize();
    set_blend_state(pso_desc.BlendState);
    pso_desc.SampleMask = UINT_MAX;
    set_rasterizer_state(pso_desc.RasterizerState);
    set_depth_stencil_state(pso_desc.DepthStencilState);

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

    ID3D12PipelineState* pipeline_state = nullptr;
    hr = device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state));

    vertex_shader->Release();
    vertex_shader = nullptr;
    pixel_shader->Release();
    pixel_shader = nullptr;

    //Model model = load_model_from_obj("cube.obj");
    Model model = load_model_from_obj();

    // shared mem = both cpu & gpu 
    // want to move to buffs which only on gpu mem (vram?)
    ID3D12Resource* vertex_buffer = nullptr; //fast. GPU access only
    ID3D12Resource* vertex_buffer_upload = nullptr; //slow. CPU and GPU access
    ID3D12Resource* index_buffer = nullptr; //fast. GPU access only
    ID3D12Resource* index_buffer_upload = nullptr; //slow. CPU and GPU access

    // default heap = on VRAM (GPU only) so fast 
    // upload heap -> CPU can write to and slower to read from for GPU (shared mem?)
    // so want to throw things in default for ease

    //create vertex and index buffer
    {
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
        resource_desc.Width = sizeof(Vertex) * model.vertices.size();
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
        resource_desc.Width = sizeof(unsigned int) * model.indicies.size();
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
        void* vertex_mapped_data = nullptr;
        hr = vertex_buffer_upload->Map(0, nullptr, &vertex_mapped_data);
        memcpy(vertex_mapped_data, model.vertices.data(), sizeof(Vertex) * model.vertices.size());
        vertex_buffer_upload->Unmap(0, nullptr);

        void* index_mapped_data = nullptr;
        hr = index_buffer_upload->Map(0, nullptr, &index_mapped_data);
        memcpy(index_mapped_data, model.indicies.data(), sizeof(unsigned int) * model.indicies.size());
        index_buffer_upload->Unmap(0, nullptr);

        // Create upload heap for constant buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = (sizeof(MVPConstants) + 255) & ~255; // 256-byte aligned
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&constantBuffer)
        );

        // Map once and keep mapped
        constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedCB));

        // adding commands to cmd list to do the copying 
        // barrier putting them in a tem moving state

        //Record commands to copy the data from the upload buffer to the fast default buffer
        hr = command_allocator->Reset();
        hr = command_list->Reset(command_allocator, nullptr);

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

        command_list->ResourceBarrier(2, barrier);

        //copy the data from upload to the fast default buffer
        command_list->CopyResource(vertex_buffer, vertex_buffer_upload);
        command_list->CopyResource(index_buffer, index_buffer_upload);

        barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

        barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

        command_list->ResourceBarrier(2, barrier);

        hr = command_list->Close();

        ID3D12CommandList* command_lists[] = { command_list };
        command_queue->ExecuteCommandLists(1, command_lists);

        // Wait on the CPU for the GPU frame to finish
        const UINT64 current_fence_value = ++fence_value;
        hr = command_queue->Signal(fence, current_fence_value);

        if (fence->GetCompletedValue() < current_fence_value) {
            hr = fence->SetEventOnCompletion(current_fence_value, fence_event);
            WaitForSingleObject(fence_event, INFINITE);
        }
    }

    unsigned int triangle_angle = 10;

    bool running = true;
    while (running) {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Record commands to draw a triangle
        hr = command_allocator->Reset();
        hr = command_list->Reset(command_allocator, nullptr);

        UINT back_buffer_index = swap_chain->GetCurrentBackBufferIndex();

        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = render_targets[back_buffer_index];
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            command_list->ResourceBarrier(1, &barrier);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
        rtv_handle.ptr += back_buffer_index * rtv_increment_size;

        // Clear the render target
        float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        command_list->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_heap->GetCPUDescriptorHandleForHeapStart();
        command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // Set viewport and scissor
        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
        D3D12_RECT scissor_rect = { 0, 0, width, height };
        command_list->RSSetViewports(1, &viewport);
        command_list->RSSetScissorRects(1, &scissor_rect);

        command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
        command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list->SetGraphicsRootSignature(root_signature);
        command_list->SetPipelineState(pipeline_state);

        // Draw the triangle
        //command_list->SetGraphicsRoot32BitConstant(0, triangle_angle, 0);

        D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
        vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
        vertex_buffer_view.StrideInBytes = sizeof(Vertex);
        vertex_buffer_view.SizeInBytes = sizeof(Vertex) * model.vertices.size();
        command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);

        D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
        index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
        index_buffer_view.SizeInBytes = sizeof(unsigned int) * model.indicies.size();
        index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
        command_list->IASetIndexBuffer(&index_buffer_view);

        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = constantBuffer->GetGPUVirtualAddress();
        command_list->SetGraphicsRootConstantBufferView(0, cbAddress);

        command_list->DrawIndexedInstanced(model.indicies.size(), 1, 0, 0, 0);

        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = render_targets[back_buffer_index];
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            command_list->ResourceBarrier(1, &barrier);
        }

        hr = command_list->Close();

        ID3D12CommandList* command_lists[] = { command_list };
        command_queue->ExecuteCommandLists(1, command_lists);

        hr = swap_chain->Present(1, 0);

        // Wait on the CPU for the GPU frame to finish
        const UINT64 current_fence_value = ++fence_value;
        hr = command_queue->Signal(fence, current_fence_value);

        if (fence->GetCompletedValue() < current_fence_value) {
            hr = fence->SetEventOnCompletion(current_fence_value, fence_event);
            WaitForSingleObject(fence_event, INFINITE);
        }

        triangle_angle++;

        // model matrix 
        float angle = DirectX::XMConvertToRadians(triangle_angle);
        DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixRotationY(angle);

        // View matrix 
        DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
        DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, at, up);

        // Projection matrix
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(60.0f), aspect, 0.5f, 100.0f);

        DirectX::XMMATRIX mvp = modelMatrix * view * proj;
        cbData.mvp = XMMatrixTranspose(mvp); // transpose for HLSL row-major
        *mappedCB = cbData;
    }
}