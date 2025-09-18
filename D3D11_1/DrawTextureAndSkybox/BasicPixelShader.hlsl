#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET 
{
    // texture
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);
    
    // color
    float4 finalColor = 0;    
    finalColor = finalTexture * saturate(dot((float3)-LightDirection, input.Norm) * LightColor);
    finalColor.a = 1;    
    
    return finalColor;
} 