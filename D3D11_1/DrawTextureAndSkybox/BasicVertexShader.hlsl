#include <Shared.fxh>

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Norm = normalize(mul(input.Norm, (float3x3) World));
    output.Tex = input.Tex;
    
    return output;
}
 