#include <Shared.fxh>

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    // pos ±¸ÇÏ±â
    output.Pos = mul(input.Pos, World);
    output.World = output.Pos;    
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    // normal
    output.Norm = normalize(cross(input.Tangent, input.Bitangent));
    output.Tangent = input.Tangent;
    output.Bitangent = input.Bitangent;
    
    // texure
    output.Tex = input.Tex;
    
    return output;
}
 