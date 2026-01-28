#include "Shared.fxh"

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.Pos = mul(float4(input.Pos, 1.0f), WVP);
    output.TexCoord = input.TexCoord;

    return output;
}