#include <Shared.fxh>

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 sampleTex = imageTex.Sample(ObjSamplerState, input.TexCoord);
    
    float4 final = sampleTex * ImageBaseColor;
    
    return final;
}
