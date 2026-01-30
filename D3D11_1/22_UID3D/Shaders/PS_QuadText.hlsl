#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{
    float a = textAtlas.Sample(ObjSamplerState, input.TexCoord).r;
    // return float4(ImageBaseColor.rgb, ImageBaseColor.a * a);
    return float4(a, a, a, 1);
}