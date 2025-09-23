#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);
    
    float3 reflictVec = { 0, 0, 0 };
    
    ComputeDirectionalLight((float3)LightDirection, input.Norm, reflictVec); // R
    
    // float3 viewDiection = 
    // I(sepc) = (R dot V)^a -> (นÝป็ บคลอ dot บไ นๆวโบคลอ)^ฑคลรม๖ผ๖
    
    
    float4 finalColor = finalTexture * (1.0f, 1.0f, 1.0f, 1.0f); // ???
    
    return finalColor;
}