cbuffer Transforms : register(b0)
{
    float4x4 mvp;
}

struct VertexOutput {
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

VertexOutput main(float3 pos : MY_POS, float2 texCoord : TEXCOORD, float3 normal : NORMAL)
{
    VertexOutput output;
    output.position = mul(float4(pos, 1.0), mvp);
    output.texCoord = texCoord;
    output.normal = normal;

    return output;
}