#include <Shared.fxh>

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    // model -> world space
    Matrix finalWorld = mul(bonePose[refBoneIndex], World);
    output.Pos = mul(input.Pos, finalWorld);
    output.World = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);    
    
    output.Norm = normalize(mul(input.Norm, (float3x3) finalWorld));
    output.Tangent = normalize(mul(input.Tangent, (float3x3) finalWorld));
    output.Bitangent = normalize(mul(input.Bitangent, (float3x3) finalWorld));
    
    // texure
    output.Tex = input.Tex;
    
    return output;
}
 