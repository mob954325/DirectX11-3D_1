#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = lerp(UVRect.xy, UVRect.zw, input.TexCoord);
    float a = textAtlas.Sample(ObjSamplerState, uv).r;
    // return float4(ImageBaseColor.rgb, ImageBaseColor.a * a);
    return float4(a, a, a, 1);
}