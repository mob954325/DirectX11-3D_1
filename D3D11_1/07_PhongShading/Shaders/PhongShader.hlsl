#include <Shared.fxh>
// Phong
// I = k_a * l_a + k_d(N dot L) I_l + k_s (R dot V)^a * I_l   
// 최종 출력 값 = 환경광 + 확산광 + 정반사광    
// 빛의 광들과 머터리얼의 광들을 dot의 각도에 따라서 색을 결정함    

float4 main(PS_INPUT input) : SV_TARGET
{       
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);     
    float3 norm = normalize(input.Norm);

    float4 finalAmbient = matAmbient * LightAmbient;   
    
    float diffuseFactor = dot(-(float3)LightDirection, norm);
    
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    if (diffuseFactor > 0.0f)
    {   
        // Phong
        float3 reflectionVector = reflect(normalize((float3)LightDirection), norm); // 
        
        // reflection calculates 
        // float3 i = normalize((float3) LightDirection);
        // float3 n = norm; 
        // float3 reflectionVector = i - 2 * n * dot(i, n);
        
        float specFactor = pow(max(dot(reflectionVector, normalize(CameraPos - input.World)), 0.0f), Shininess);
        
        finalDiffuse = finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = specFactor * matSpecular * LightSpecular * LightColor;
    }  
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular;
    finalColor.a = finalTexture.a;
    
    return finalColor;

}