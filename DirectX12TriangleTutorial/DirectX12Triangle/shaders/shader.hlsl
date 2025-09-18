struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

cbuffer MVPBuffer : register(b0)
{
    float4x4 mvp;
};

// Vertex Shader
PSInput VSMain(VSInput input) {
    PSInput output;

    // flip order if remove transpose
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.normal = input.normal;

    return output;
}

// Pixel Shader
float4 PSMain(PSInput input) : SV_TARGET{
    return float4(abs(input.normal), 1.0f);
}