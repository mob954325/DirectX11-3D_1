#include <Shared.fxh>

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    // pos 구하기
    Matrix finalWorld = mul(modelMatricies[refBoneIndex], World);
    // Matrix finalWorld = modelMatricies[refBoneIndex];
    output.Pos = mul(input.Pos, finalWorld);
    output.World = output.Pos;    
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    // normal
    // output.Norm = input.Norm;
    // output.Tangent = input.Tangent;
    // output.Bitangent = input.Bitangent;
    
    output.Norm = normalize(mul(input.Norm, (float3x3) finalWorld));
    output.Tangent = normalize(mul(input.Tangent, (float3x3) finalWorld));
    output.Bitangent = normalize(mul(input.Bitangent, (float3x3) finalWorld));
    
    // texure
    output.Tex = input.Tex;
    
    return output;
}
 