#include "Shared.hlsli"

PS_INPUT_QUAD main(VS_INPUT_QUAD input)
{    
    PS_INPUT_QUAD output = (PS_INPUT_QUAD) 0;
    output.position = float4(input.position.xy, 0.0f, 1.0f);
    output.tex = input.tex;
    
    return output;
}