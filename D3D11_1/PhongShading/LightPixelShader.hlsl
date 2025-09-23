#include <Shared.fxh>

float4 main(PS_INPUT input) : SV_TARGET
{       
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);    
    
    // Phong
    // I = k_a * l_a + k_d(N dot L) I_l + k_s (R dot V)^a * I_l   
    // 최종 출력 값 = 환경광 + 확산광 + 정반사광    
    // 빛의 광들과 머터리얼의 광들을 dot의 각도에 따라서 색을 결정함
    
    float3 lightDir = -LightDirection;
    
    float4 finalAmbient = matAmbient * LightAmbient;   
    float3 norm = normalize(input.Norm);
    float diffuseFactor = dot(lightDir, norm);   
    
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    if (diffuseFactor > 0.0f)
    {    
        float3 v = normalize(reflect((float3) LightDirection, norm)); // reflect값이 정규화된 값이 아님 
        float specFactor = pow(max(dot(v, normalize(CameraPos - input.World)), 0.0f), Shininess);
    
        finalDiffuse = diffuseFactor * matDiffuse * LightDiffuse;
        finalSpecular = specFactor * matSpecular * LightSpecular;
    }  
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular;
    
    return finalColor * finalTexture;
}