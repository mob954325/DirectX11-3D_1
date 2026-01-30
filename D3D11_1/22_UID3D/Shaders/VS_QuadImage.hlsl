#include "Shared.fxh"

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    output.Pos = mul(float4(input.Pos, 1.0f), imageWVP);
    output.TexCoord = input.TexCoord;

    return output;
}