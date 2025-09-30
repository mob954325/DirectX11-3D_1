#include "Shared.fxh"

PS_INPUT_Sky main(VS_INPUT_sky input)
{	
    PS_INPUT_Sky output = (PS_INPUT_Sky) 0;
    
    float4 pos = float4(input.Pos, 1.0f);
    Matrix ViewWithoutTranlation = View;
    
    ViewWithoutTranlation._41 = 0;
    ViewWithoutTranlation._42 = 0;
    ViewWithoutTranlation._43 = 0;
    
    pos = mul(pos, ViewWithoutTranlation);
    pos = mul(pos, Projection);    
    
    output.positionClipSpace = pos;
    output.position = normalize(input.Pos);     
    
	return output;
}