#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET 
{
	float4 textureColor = txDiffuse.Sample(samLinear, input.Tex);
    if (!hasDiffuse)
    {
        textureColor = (float4) (0.7, 0.7, 0.7, 1);
    }
	
	return textureColor;
} 