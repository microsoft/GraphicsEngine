cbuffer MVPConstants : register(b0)
{
    float4x4 mvp;
    float4x4 model; // Add model matrix
    float4x4 normalMatrix; // Add normal matrix (transpose of inverse model)
}

cbuffer MaterialConstants : register(b1)
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float shininess;
    float3 padding;
}

Texture2D diffuseTexture : register(t0);
SamplerState samplerState : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.worldPos = mul(float4(input.position, 1.0f), model).xyz;
    // normalMatrix supplied as inverse(model) (or inverse-transpose). Use it to correctly transform normals.
    output.normal = mul((float3x3)normalMatrix, input.normal);
    output.uv = input.uv;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // Sample texture
    float4 textureColor = diffuseTexture.Sample(samplerState, input.uv);
    if (textureColor.a < 0.1f) discard;
    // Lighting
    float3 N = normalize(input.normal);
    // Make foliage two-sided: if backface, flip normal so leaves get light on both sides
    if (dot(N, float3(0.0f,1.0f,0.0f)) < 0) N = -N;
    float3 lightDir = normalize(float3(-0.4f, -1.0f, 0.3f)); // light coming from above
    float NdotL = saturate(dot(N, -lightDir)); // since lightDir points from light to surface

    // Enforce a minimum ambient so dark textures still visible
    float3 amb = max(ambient.rgb, float3(0.12f, 0.12f, 0.12f));
    float3 diffTerm = diffuse.rgb * NdotL;
    float3 color = textureColor.rgb * (amb + diffTerm);

    // Optional simple gamma correction
    color = pow(color, 1.0f/2.2f);
    return float4(color, textureColor.a);
}