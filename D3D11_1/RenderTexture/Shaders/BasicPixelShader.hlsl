#include "Shared.fxh"

float4 main(PS_INPUT input) : SV_TARGET
{
    return txDiffuse.Sample(samLinear, input.Tex);
} 