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
    float materialKd;
    float materialKs;
    float materialN;
    float materialKa;
    // need to define members in this order since float3 takes up an entire register even though it's 12 bytes
    float3 materialDiffuse;
};

cbuffer LightCB : register(b5)
{
    float3 lightPos;
    float _pad1; // Padding to 16B
    float3 lightColor;
    float _pad2; // Padding to 16B
};

float4 main(VertexOutput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(lightPos - input.worldPosition);
    float lambertian = max(dot(N, L), 0.0);
    float specular = 0.0;
    if (lambertian > 0.0)
    {
        float3 V = normalize(cameraPos - input.worldPosition); // Vector to viewer
        float3 H = normalize(L + V); // Blinn-Phong, use halfway vector instead of Reflection vector
        // Compute the specular term
        float specAngle = max(dot(N, H), 0.0);
        specular = pow(specAngle, materialN);
    }
    float3 albedo = colourTex.Sample(colourSampler, input.texCoords);
    return float4(materialKa * albedo + materialKd * lambertian * albedo * lightColor + materialKs * specular * lightColor, 1.0f);
}