#include <Shared.hlsli>

PS_QuadInput main(VS_QuadInput input)
{
    PS_QuadInput output;
    output.position = float4((float2)input.position, 0, 1.0f);
    output.tex = input.tex;
    
    return output;
}