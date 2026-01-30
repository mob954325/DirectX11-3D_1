#include <Shared.fxh>

PS_MESH_INPUT main(float4 Pos : POSITION, float4 Color : COLOR)
{
    PS_MESH_INPUT output = (PS_MESH_INPUT) 0;
    output.Pos = mul(Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Color = Color;
    return output;
}
 