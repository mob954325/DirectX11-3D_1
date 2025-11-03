#include <Shared.fxh>

Texture2D shadowMap : register(t0);
SamplerState samplerState : register(s0);

float4 main(PS_INPUT input) : SV_Target
{
    float d = shadowMap.Sample(samplerState, input.ShadowUV).r;
    return float4(d, d, d, 1.0f); // depth 값을 흑백으로 표시
}
