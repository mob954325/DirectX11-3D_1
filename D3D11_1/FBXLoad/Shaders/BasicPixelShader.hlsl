#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET 
{
	float4 textureColor = txDiffuse.Sample(samLinear, input.Tex);
	float4 textureEmission = txEmission.Sample(samLinear, input.Tex);

	if(textureColor.a < 0.5f) discard;

	return textureColor + textureEmission;
} 