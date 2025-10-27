#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{
    // specularSample
    float specularIntensity = txSpec.Sample(samLinear, input.Tex).r;   
    if (!hasSpecular)
    {
        specularIntensity = 1.0f;
    }
    
    // EmissionSample
    float4 textureEmission = txEmission.Sample(samLinear, input.Tex);
    if (!hasEmissive)
    {
        textureEmission = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    // normalSample
    float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Norm);    
    float3 normalMapSample = txNormal.Sample(samLinear, input.Tex).rgb;
    // normal map이 없을 경우를 대비
    if (!hasNormal)
    {
        normalMapSample = float3(0.5f, 0.5f, 1.0f); // flat normal (no perturbation)
    }
    float3 normalTexture = normalize(DecodeNormal(normalMapSample)); //  Convert normal map color (RGB [0,1]) to normal vector in range [-1,1] 
    
    float3 finalNorm = normalize(mul(normalTexture, TBN));
    
    // texture Sampling 
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);
    
    // lighting Calculate
    float3 norm = finalNorm; // normal
	
    float4 finalAmbient = matAmbient * LightAmbient;	
    
    float diffuseFactor = dot((float3) LightDirection, norm);
	
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
	if(diffuseFactor > 0.0f)
    {        
        // 정반사광        
        float3 viewVector = CameraPos - input.World;        
        float3 halfVector = viewVector + -(float3)LightDirection;        
        float specularFactor = specularIntensity * pow(saturate(dot(norm, normalize(halfVector))), Shininess);
        
        finalDiffuse = finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = specularFactor * matSpecular * LightSpecular * LightColor;
    }
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular; // 최종 색
    finalColor.a = finalTexture.a;        
    
    return finalColor + textureEmission;
}