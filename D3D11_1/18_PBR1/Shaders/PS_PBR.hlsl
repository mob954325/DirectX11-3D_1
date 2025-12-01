#include <Shared.fxh>

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

// D(h) (Normal Distribution Function)
float NdfGGXTR(float3 normal, float3 halfVec, float roughness)
{
    float alpha = roughness * roughness;
    float demon = PI * pow((pow(dot(normal, halfVec), 2) * (alpha - 1) + 1), 2);
    
    return alpha / demon;
}

// F(v, h)
float3 FresnelReflection()
{
    return float3(0, 0, 0);
}

// G(l,v,h)
float GeometricAttenuation()
{
    return float(0);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    // 그림자처리 부분 =====================================================================
    float directLighing = 1.0f;
    // 광원 NDC 좌표계에서는 좌표는 계산해주지 않으므로 계산한다.
    float currentShadowDepth = input.PositionShadow.z / input.PositionShadow.w;
    
    // 광원 NDC 좌표계에서의 x(-1 ~ 1), y(-1 ~ 1)
    float2 uv = input.PositionShadow.xy / input.PositionShadow.w;
    
    // NDC좌표계에서 Texture 좌표계로 변환
    uv.y = -uv.y;
    uv = uv * 0.5 + 0.5;
    
    if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
    {
        float sampleShadowDepth = txShadow.Sample(samLinear, uv).r;
        
        // currentShadowDepth가 더 크면 뒤 쪽에 있으므로 직접광 차단
        if (currentShadowDepth > sampleShadowDepth + 0.001)
        {
            directLighing = 0.0f;
        }
    }
    
    // 광원처리 부분 =====================================================================
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
    
    if (finalTexture.a < 0.5f)
        discard;
    
    // lighting Calculate
    float3 norm = finalNorm; // normal
    float4 lightDir = -LightDirection;
    
    float4 finalAmbient = matAmbient * LightAmbient;
    
    float diffuseFactor = dot((float3) lightDir, norm);
    
    float4 finalDiffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 finalSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    if (diffuseFactor > 0.0f)
    {
        // 정반사광        
        float3 viewVector = CameraPos - input.World;
        float3 halfVector = viewVector + -(float3) lightDir;
        float specularFactor = specularIntensity * pow(saturate(dot(norm, normalize(halfVector))), Shininess);
        
        float d = NdfGGXTR(norm, halfVector, Roughness);
        
        finalDiffuse = d * finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
        finalSpecular = d * specularFactor * matSpecular * LightSpecular * LightColor;
    }
    
    float4 finalColor = directLighing * (finalAmbient + finalDiffuse + finalSpecular); // 최종 색
    finalColor.a = finalTexture.a;
    
    return finalColor + textureEmission;
}