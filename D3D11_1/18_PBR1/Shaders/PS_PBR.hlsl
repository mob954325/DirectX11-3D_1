#include <Shared.fxh>

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

// D(h) (Normal Distribution Function) : Trowbridge-Reitz GGX
float NDFGGXTR(float3 normal, float3 halfVec, float roughness)
{
    float cosLh = dot(normal, halfVec);
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    
    float demon = PI * pow((cosLh * cosLh) * (alphaSq - 1) + 1, 2);
    
    return alphaSq / demon;
}

// F(v, h) Fresnel equation : Fresnel-Schlick approximation 
float3 FresnelSchlick(float3 halfVec, float3 viewVec, float3 F0) // F0 == relfection factor
{
    return F0 + (1.0 - F0) * pow(1.0 - dot(halfVec, viewVec), 5.0);  
}

// G(l,v,h) 
float GSchlickGGX(float3 norm, float3 viewVec, float roughness)
{
    float NdotV = dot(norm, viewVec);    
    float denom = NdotV * (1.0 - roughness) + roughness;
    return NdotV / denom;
}

// G(n, v, l, k) : Smith's Method
float GSmithMethod(float3 norm, float3 viewVec, float3 lightVec, float roughness)
{
    float kDir = (roughness + 1) * (roughness + 1);     // for directional lighting
    //float kIBL = roughness* roughness / 2.0;          // IBL lighting
    kDir /= 8.0;
    
    return GSchlickGGX(norm, viewVec, kDir) * GSchlickGGX(norm, lightVec, kDir);
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
    float4 albedo = txDiffuse.Sample(samLinear, input.Tex);
    if (!hasDiffuse)
    {
        albedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    if (albedo.a < 0.5f)
        discard;
    
    float3 directLighting = 0.0f;
    
    // Cook-Torrance Specular BRDF
    float3 norm = finalNorm;
    float3 lightIn = (float3)LightDirection;    
    
    float3 Lo = normalize(CameraPos - (float3) input.World); // 빛이 눈으로 가는 방향 : 현재 위치 -> eye ( view Vector )
    float3 Li = -(float3)LightDirection;    // 빛 방향
    float3 Lh = normalize(Li + Lo); // half-vector between Li and Lo        
    
    float NdotL = max(0.0, dot(norm, Li));
    float NdotO = max(0.0, dot(norm, Lo));
    
    // 기본 반사율(F0) = lerp(비금속 평균 반사, baseColor(텍스처), matalness)
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), (float3)albedo, Metalness);   
    
    float D = NDFGGXTR(norm, Lh, max(0.001, Roughness));
    float F = FresnelSchlick(Lh, Lo, F0);
    float G = GSmithMethod(norm, Lo, Li, Roughness);    
    
    float3 specularBRDF = (D * F * G) / max(Epsilon, 4.0 * NdotL * NdotO);
   
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), Metalness);
    
    // Lambert diffuse BRDF  
    float3 diffuseBRDF = kd * (float3)albedo / PI;
    
    directLighting = (diffuseBRDF + specularBRDF) * NdotL;    
    
    return float4(pow(float3(directLighting), 1.0 / 2.2), 1.0);
}