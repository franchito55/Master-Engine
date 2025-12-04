Texture2D colourTex : register(t0);
SamplerState colourSampler : register(s0);
struct VertexOutput {
    float2 texCoords : TEXCOORD;
    float4 position : SV_POSITION;
};
float4 main(VertexOutput vertexOutput) : SV_TARGET
{
    return colourTex.Sample(colourSampler, vertexOutput.texCoords);
}