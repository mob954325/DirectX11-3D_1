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
    if (!hasDiffuse)
    {
        finalTexture = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // lighting Calculate
    float3 norm = finalNorm; // normal
    float4 lightDir = -LightDirection;
	
    float4 finalAmbient = matAmbient * LightAmbient;
    
    float NdotL = dot((float3) lightDir, norm);
    float lightIntensity = smoothstep(0.0f, 0.01f, NdotL); // 어두운 부분과 밝은 부분을 나눔
    // float stepped = ceil(NdotL * 3.0) / 3.0; // 명암 나누기
    float diffuseFactor = lightIntensity;
	
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float3 viewVector = CameraPos - input.World; // 표면에서 카메라에 대한 방향벡터
    if (diffuseFactor > 0.0f)
    {
        // 정반사광        
        float3 halfVector = viewVector + -(float3) lightDir;
        float specularFactor = specularIntensity * pow(saturate(dot(norm, normalize(halfVector))), Shininess);
        float specularFactorSmooth = smoothstep(0.005f, 0.01f, specularFactor); // 정반사광이 좀 더 날카롭게 표현하기위해 사용
        
        finalDiffuse = finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = specularFactorSmooth * matSpecular * LightSpecular * LightColor;
    }
    
    // Rim light
    float4 rimColor = float4(0, 1, 0, 1); // green
    float rimAmount = 0.725f;
    float rimThreshold = 0.01f; // 0 - 1
    
    float3 viewDir = normalize(viewVector);
    float rimDot = 1 - dot(viewDir, norm);
    
    float rimFactor = rimDot * pow(NdotL, rimThreshold); // 빛 반사율에 대한 rim 강도
    // float rimFactor = rimDot * pow(rimDot, rimThreshold) * saturate(NdotL + 0.2f); // 표면과 view에 대한 rim강도 - 실험
    rimFactor = smoothstep(rimAmount - 0.01f, rimAmount + 0.01f, rimFactor);
    float4 rimFinal = rimFactor * rimColor;
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular + rimFinal; // 최종 색
    finalColor.a = finalTexture.a;
    
    return finalColor + textureEmission;
}