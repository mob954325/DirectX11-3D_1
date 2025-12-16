#include <Shared.hlsli>

float main(PS_INPUT input) : SV_Depth
{
    float alpha = txDiffuse.Sample(samLinear, input.Tex).a;
    if(!hasDiffuse)
    {
        alpha = 1.0f;
        return input.Pos.z;
    }
    
    clip(alpha - 0.5f);
    
    return input.Pos.z; // 색상 기록은 안함.
}