#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET 
{
	float4 textureColor = txDiffuse.Sample(samLinear, input.Tex);
	
	return textureColor;
} 