#include <Shared.fxh>

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return imageTex.Sample(ObjSamplerState, input.TexCoord);
}
