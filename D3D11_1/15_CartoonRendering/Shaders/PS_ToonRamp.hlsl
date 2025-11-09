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
	
    float4 finalAmbient = matAmbient * LightAmbient;
    
    float NdotL = dot((float3) LightDirection, norm);

    // diffuse Ramp Texture 
    float2 diffuseRampUV = float2(NdotL, 0.5f);
    float4 diffuseRampSample = txDiffuseRamp.Sample(samLinear, diffuseRampUV);
	
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float3 viewVector = CameraPos - input.World; // 표면에서 카메라에 대한 방향벡터
    if (NdotL > 0.0f)
    {
        // 정반사광        
        float3 halfVector = viewVector + -(float3) LightDirection;
        float specularFactor = specularIntensity * pow(saturate(dot(norm, normalize(halfVector))), Shininess);
        float specularFactorSmooth = smoothstep(0.005f, 0.01f, specularFactor); // 정반사광이 좀 더 날카롭게 표현하기위해 사용
        
        float2 specRampUV = float2(specularFactor, 0.5f);
        float4 specRampSample = txSpecRamp.Sample(samLinear, specRampUV);

        finalDiffuse = finalTexture * diffuseRampSample * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = specularFactorSmooth * specRampSample * matSpecular * LightSpecular * LightColor;
    }
    
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular; // 최종 색
    finalColor.a = finalTexture.a;
    
    return finalColor + textureEmission;
}