#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET 
{
    float4 finalColor = 0;
    float4 BaseColor = (1.0f, 1.0f, 1.0f, 1.0f);
    finalColor = BaseColor * saturate(dot((float3) LightDirection, input.Norm) * LightColor);
    
    finalColor.a = 1;
    
    return finalColor;
} 