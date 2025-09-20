cbuffer MVPConstants : register(b0)
{
    float4x4 mvp;
    float4x4 model;
    float4x4 normalMatrix;
    float3 viewPos;
    float _padView;
}

cbuffer MaterialConstants : register(b1)
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float shininess;
    float3 padding;
    
    // Fog parameters
    float fogStart;
    float fogEnd;
    float fogDensity;
    float _padFog;
    
    // Flashlight parameters
    float3 flashlightPos;
    float flashlightRange;
    float3 flashlightDir;
    float flashlightAngle;
    float flashlightIntensity;
    float3 _padFlash;
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
    output.normal = mul((float3x3)normalMatrix, input.normal);
    output.uv = input.uv;
    return output;
}

float ComputeFog(float distance)
{
    // Exponential fog for horror atmosphere
    return 1.0 - exp(-fogDensity * distance);
}

float ComputeFlashlight(float3 worldPos, float3 normal)
{
    float3 toLight = flashlightPos - worldPos;
    float distance = length(toLight);
    
    // Outside flashlight range
    if (distance > flashlightRange) return 0.0;
    
    float3 lightDir = normalize(toLight);
    
    // Check if point is within flashlight cone
    float spotCos = dot(-lightDir, flashlightDir);
    float angleCos = cos(flashlightAngle * 0.5);
    
    if (spotCos < angleCos) return 0.0;
    
    // Smooth falloff at cone edges
    float spotFactor = smoothstep(angleCos, angleCos + 0.1, spotCos);
    
    // Distance attenuation
    float attenuation = 1.0 - smoothstep(flashlightRange * 0.5, flashlightRange, distance);
    
    // Normal-based lighting
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    return spotFactor * attenuation * NdotL * flashlightIntensity;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // Sample texture
    float4 textureColor = diffuseTexture.Sample(samplerState, input.uv);
    if (textureColor.a < 0.1f) discard;
    
    float3 N = normalize(input.normal);
    
    // Base ambient lighting (very dark for horror)
    float3 lightDir = normalize(float3(-0.4f, -1.0f, 0.3f));
    float NdotL = saturate(dot(N, -lightDir));
    
    // Very dark ambient for horror atmosphere
    float3 amb = float3(0.02f, 0.02f, 0.03f); // Slightly blue tinted darkness
    float3 diffTerm = diffuse.rgb * NdotL * 0.1; // Reduced general lighting
    
    // Base color
    float3 color = textureColor.rgb * (amb + diffTerm);
    
    // Add flashlight illumination
    float flashlightFactor = ComputeFlashlight(input.worldPos, N);
    color += textureColor.rgb * flashlightFactor * float3(1.0f, 0.95f, 0.8f); // Warm flashlight color
    
    // Apply fog
    float distance = length(input.worldPos - viewPos);
    float fogFactor = ComputeFog(distance);
    
    // Fog color (very dark, almost black)
    float3 fogColor = float3(0.01f, 0.01f, 0.02f); // Near black with slight blue tint
    color = lerp(color, fogColor, fogFactor);
    
    // Subtle gamma correction
    color = pow(color, 1.0f/2.2f);
    
    return float4(color, textureColor.a);
}