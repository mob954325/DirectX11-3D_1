#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{
    float a = textAtlas.Sample(textUISamplerState, input.TexCoord).r;
    return float4(ImageBaseColor.rgb, ImageBaseColor.a * a);
}