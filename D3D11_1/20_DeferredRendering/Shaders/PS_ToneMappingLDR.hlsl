#include "Shared.hlsli"

float3 LinearToSRGB(float3 linearColor)
{
    return pow(linearColor, 1.0f / 2.2f);
}

float4 main(PS_QuadInput input) : SV_Target
{
     // 1. 선형 HDR 값 로드 (Nits 값으로 간주)
    float3 C_linear709 = txSceneHDR.Sample(samLinear, input.tex).rgb * lightIntensity;
    
    float exposureFactor = pow(2.0f, exposure);
    C_linear709 *= exposureFactor;

    float3 C_tonemapped;
    C_tonemapped = ACESFilm(C_linear709);
   
    float3 C_final;
    C_final = LinearToSRGB(C_tonemapped);
    
    if (useToneMapping) return float4(pow(C_linear709, 1.0 / 2.2), 1.0);
        
    return float4(C_final, 1.0);
}
