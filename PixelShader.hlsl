static const float PI = 3.14159265f;

Texture2D colourTex : register(t0);

SamplerState colourSampler : register(s0);

struct VertexOutput {
    float2 texCoords : TEXCOORD0;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPosition : WORLDPOS;
};

cbuffer CameraCB : register(b3)
{
    float3 cameraPos; // 12B
    float _pad0; // Pad to 16 bytes (must do with cbuffers)
};

cbuffer MaterialCB : register(b4)
{
    float3 materialRf0;
    float _pad3; // Padding to 16B
    float3 materialDiffuse;
    float _pad4;
    float materialN;
};

cbuffer LightCB : register(b5)
{
    float3 lightPos;
    float _pad1; // Padding to 16B
    float3 lightColor;
    float _pad2; // Padding to 16B
};

float3 maxRGB(float3 color)
{
    float maxValue = max(max(color.r, color.g), color.b);
    return float3(maxValue, maxValue, maxValue);
}

float3 FresnelSchlick(float cosTheta)
{
    return materialRf0 + (1 - materialRf0) * pow(1 - cosTheta, 5);
}

float4 main(VertexOutput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(lightPos - input.worldPosition);
    float3 V = normalize(cameraPos - input.worldPosition);
    float3 H = normalize(L + V);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    float3 albedo = colourTex.Sample(colourSampler, input.texCoords) * materialDiffuse;
    
    // Diffuse -> reflected color (not specular)
    float3 diffuse = albedo * (1 - maxRGB(materialRf0)) / PI;
    
    // Fresnel -> how much of the reflected light is specular?
    float3 F = FresnelSchlick(VdotH);
    
    // Specular
    float specular = ((materialN + 2) / (2 * PI)) * F * pow(NdotH, materialN);
    
    float3 color =
        (diffuse + specular) // BSDF
        * lightColor * NdotL;
    
    return float4(color, 1.0);
}