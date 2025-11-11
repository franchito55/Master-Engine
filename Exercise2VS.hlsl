float4 main(float3 pos : MY_POS) : SV_POSITION
{
    return float4(pos, 1.0);
}