#include "Shared.hlsli"

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
    pos.z = pos.w; // 항상 뒤에서 그리기 (fixed depth)
    
    output.PosClipSpace = pos;
    output.Pos = normalize(input.Pos);     
    
	return output;
}