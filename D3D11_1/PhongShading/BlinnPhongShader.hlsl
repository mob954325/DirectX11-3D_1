#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);
    float3 norm = normalize(input.Norm);	
	
    float4 finalAmbient = matAmbient * LightAmbient;	
    
    float diffuseFactor = dot(-(float3) LightDirection, norm);
	
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
	if(diffuseFactor > 0.0f)
    {        
        // 정반사광        
        float3 viewVector = CameraPos - input.World;        
        float3 halfVector = viewVector + -(float3)LightDirection;        
        float specularFactor = pow(saturate(dot(norm, normalize(halfVector))), Shininess);
        
        finalDiffuse = finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = specularFactor * matSpecular * LightSpecular * LightColor;
    }
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular; // 최종 색
    finalColor.a = finalTexture.a;
    
    return finalColor;
}