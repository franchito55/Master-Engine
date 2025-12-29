Texture2D colourTex : register(t0);

SamplerState colourSampler : register(s0);

struct VertexOutput {
    float2 texCoords : TEXCOORD0;
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
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
        float3 R = reflect(-L, N); // Reflected light vector
        float3 V = normalize(cameraPos - input.worldPosition); // Vector to viewer
        // Compute the specular term
        float specAngle = max(dot(R, V), 0.0);
        specular = pow(specAngle, materialN);
        //return float4(specAngle, specAngle, specAngle, 1.0);
    }
    float3 albedo = colourTex.Sample(colourSampler, input.texCoords);
    return float4(materialKa * albedo + materialKd * lambertian * albedo * lightColor + materialKs * specular * lightColor, 1.0f);
    //return float4(albedo, 1.0f);
    //return float4(normalize(input.normal) * 0.5 + 0.5, 1.0);
    //return float4(specular, specular, specular, 1.0);
    //return float4(1.0, 0.0, 0.0, 1.0);

}