cbuffer MvpCB : register(b0)
{
    float4x4 mvp;
}

cbuffer ModelMatrixCB : register(b1)
{
    float4x4 modelMatrix;
}

cbuffer NormalMatrixCB : register(b2)
{
    float3x3 normalMatrix;
}

struct VertexOutput {
    float2 texCoord : TEXCOORD0;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPosition : WORLDPOS;
};


VertexOutput main(float3 pos : MY_POS, float2 texCoord : TEXCOORD, float3 normal : NORMAL)
{
    VertexOutput output;

    output.position = mul(float4(pos, 1.0), mvp);
    output.texCoord = texCoord;

    output.normal = mul(normal, normalMatrix); // Normal post-multiplied
    float4 worldPos = mul(float4(pos, 1.0), modelMatrix);
    output.worldPosition = worldPos.xyz;

    return output;
}