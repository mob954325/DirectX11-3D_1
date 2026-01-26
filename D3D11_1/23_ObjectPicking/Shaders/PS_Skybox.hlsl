#include "Shared.hlsli"

float4 main(PS_INPUT_Sky input) : SV_TARGET
{
    float4 diffuse_texture = txCubemap.Sample(samLinear, input.Pos);
    return diffuse_texture;

}