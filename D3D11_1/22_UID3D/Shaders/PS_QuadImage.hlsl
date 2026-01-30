#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 sampleTex = imageTex.Sample(ObjSamplerState, input.TexCoord);
    
    float4 final = sampleTex * ImageBaseColor;
    
    return final;
}