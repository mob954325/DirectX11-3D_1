#include "Shared.hlsli"

PS_INPUT_QUAD main(VS_INPUT_QUAD input)
{    
    PS_INPUT_QUAD output;
    output.position = float4(input.position, 1.0f);
    output.tex = input.tex;
    return output;
}