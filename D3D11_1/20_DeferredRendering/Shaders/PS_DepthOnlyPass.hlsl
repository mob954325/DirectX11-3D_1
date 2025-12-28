#include <Shared.hlsli>

float main(PS_INPUT_MODEL input) : SV_Depth
{
    float alpha = txDiffuse.Sample(samLinear, input.Tex).a;
    if(!hasDiffuse)
    {
        alpha = 1.0f;
        return input.PosCS.z;
    }
    
    clip(alpha - 0.5f);
    
    return input.PosCS.z; // 색상 기록은 안함.
}