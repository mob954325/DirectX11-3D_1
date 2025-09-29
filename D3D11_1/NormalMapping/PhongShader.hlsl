#include <Shared.fxh>
// Phong
// I = k_a * l_a + k_d(N dot L) I_l + k_s (R dot V)^a * I_l   
// 최종 출력 값 = 환경광 + 확산광 + 정반사광    
// 빛의 광들과 머터리얼의 광들을 dot의 각도에 따라서 색을 결정함    

float4 main(PS_INPUT input) : SV_TARGET
{       
    // specularSample
    float specularIntensity = txSpec.Sample(samLinear, input.Tex).r;
    
    // NormalSample to world space
    float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Norm);
    float3 normalMapSample = txNormal.Sample(samLinear, input.Tex).rgb;
    float3 normalTexture = normalize(DecodeNormal(normalMapSample)); //  Convert normal map color (RGB [0,1]) to normal vector in range [-1,1]      

    float3 finalNorm = normalize(mul(normalTexture, TBN));
    
    // texture sampling
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);         
    
    // lighting Calculate
    float3 norm = finalNorm;

    float4 finalAmbient = matAmbient * LightAmbient;   
    
    float diffuseFactor = dot(-(float3)LightDirection, norm);
    
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    if (diffuseFactor > 0.0f)
    {   
        // Phong
        float3 reflectionVector = reflect(normalize((float3)LightDirection), norm); //       
        
        float specFactor = specularIntensity * pow(max(dot(reflectionVector, normalize(CameraPos - input.World)), 0.0f), Shininess);
        
        finalDiffuse =  finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = specFactor * matSpecular * LightSpecular * LightColor;
    }  
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular;
    finalColor.a = finalTexture.a;
    
    return finalColor;

}