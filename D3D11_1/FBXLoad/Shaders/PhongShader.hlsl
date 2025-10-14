#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{       
    // specularSample
    float specularIntensity = txSpec.Sample(samLinear, input.Tex).r;
    if(!hasSpecular)
    {
        specularIntensity = 1.0f;
    }
    
    // EmissionSample
    float4 textureEmission = txEmission.Sample(samLinear, input.Tex);
    if(!hasEmissive)
    {
        textureEmission = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    // NormalSample to world space
    float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Norm);    
    float3 normalMapSample = txNormal.Sample(samLinear, input.Tex).rgb;
    if (!hasNormal)
    {
        normalMapSample = float3(0.5f, 0.5f, 1.0f); // flat normal (no perturbation)
    }
    float3 normalTexture = normalize(DecodeNormal(normalMapSample)); //  Convert normal map color (RGB [0,1]) to normal vector in range [-1,1]      

    float3 finalNorm = hasNormal ? normalize(mul(normalTexture, TBN)) : normalize(input.Norm);
    
    // texture sampling
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);  
    
    // if (finalTexture.a < 0.5f) return float4(0.1f, 0.2f, 0.3f, 1.0f); // 투명하면 배경 색상으로 출력
    
    // lighting Calculate
    float3 norm = finalNorm;

    float4 finalAmbient = matAmbient * LightAmbient;   
    
    float diffuseFactor = dot((float3)LightDirection, norm);
    
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    if (diffuseFactor > 0.0f)
    {   
        // Phong
        float3 reflectionVector = reflect(normalize(-(float3)LightDirection), norm); //       
        
        float specFactor = specularIntensity * pow(max(dot(reflectionVector, normalize(CameraPos - input.World)), 0.0f), Shininess);
        
        finalDiffuse =  finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = specFactor * matSpecular * LightSpecular * LightColor;
    }  
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular;
    finalColor.a = finalTexture.a;
    
    return finalColor + textureEmission;

}