#include "Shared.fxh"

float4 main(PS_INPUT_Sky input) : SV_TARGET
{
    float4 diffuse_texture = txCubemap.Sample(samLinear, input.position);
    return diffuse_texture;

}