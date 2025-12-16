#include <Shared.hlsli>

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
float3 FresnelSchlick(float3 F0, float cosTheta) // F0 == relfection factor
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// G(l,v,h) 
float GSchlickGGX(float3 norm, float3 viewVec, float k) // K : 블러 감소 인자
{
    float NdotV = dot(norm, viewVec);    
    float denom = NdotV * (1.0 - k) + k;
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
    float finalShadow = 1.0f;
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
            finalShadow = 0.0f;
        }
    }
    
    // 광원처리 부분 =====================================================================
    // base(diffuse) texture Sampling 
    float4 albedo = txDiffuse.Sample(samLinear, input.Tex);
    if (!hasDiffuse)
    {
        albedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    if (albedo.a < 0.5f) // alpha cliping
        discard;
    
    albedo.rgb = pow(albedo.rgb, 2.2); // gamma -> linear
    
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
    
    // MetalnessSample
    float metalnessSample = txMetalness.Sample(samLinear, input.Tex).r; // grayScale
    if(!hasMetalness)
    {
        metalnessSample = 1;
    }
    
    float finalMetalness = metalnessSample * Metalness;
    
    // RoughnessSample
    float roughnessSample = txRoughness.Sample(samLinear, input.Tex).r; // grayScale
    if(!hasRoughness)
    {
        roughnessSample = hasShininess ? 1 - roughnessSample : 1;
    }
    
    float finalRoughness = roughnessSample * Roughness;

    float3 directLighting = 0.0f;
    
    // Cook-Torrance Specular BRDF
    float3 norm = finalNorm;
    
    float3 Lo = normalize(CameraPos - (float3) input.World); // 빛이 눈으로 가는 방향 : 현재 위치 -> eye ( view Vector )
    float3 Li = -(float3)LightDirection;    // 빛 방향
    float3 Lh = normalize(Li + Lo); // half-vector between Li and Lo        
    
    float NdotL = max(0.0, dot(norm, Li));  // dot(Normal, Light Direction)
    float NdotO = max(0.0, dot(norm, Lo));  // dot(Normal, View)
    
    // 기본 반사율(F0) = lerp(비금속 평균 반사, baseColor(텍스처), matalness)
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), (float3) albedo, finalMetalness);
    
    {   
        float D = NDFGGXTR(norm, Lh, max(0.001, finalRoughness));
        float F = FresnelSchlick(F0, max(0, dot(Lh, Lo)));
        float G = GSmithMethod(norm, Lo, Li, finalRoughness);
   
        // 표면 산란
        float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), finalMetalness);
    
        // Lambert diffuse BRDF  
        float3 diffuseBRDF = kd * (float3) albedo / PI;
    
        // specular BRDF
        float3 specularBRDF = (D * F * G) / max(Epsilon, 4.0 * NdotL * NdotO);
        
        directLighting = (diffuseBRDF + specularBRDF * specularIntensity) * NdotL * finalShadow + (float3)textureEmission;
    }

    
    float3 inDirectLighting = 0.0f;
    // IBL
    {
        float3 irradiance = txIBLIrradiance.Sample(samLinear, norm).rgb;
    
        // IBL diffuse    
        float3 F = FresnelSchlick(F0, NdotO);
        float3 kd = lerp(1.0 - F, 0.0, finalMetalness);
        
        float3 diffuseIBL = kd * (float3)albedo * irradiance / PI; // irradiance map이 1/pi가 포함되어있으면 제거 있으면 1/pi 추가하기
    
        // IBL specular        
        uint specularTexureLevels, width, height;        
        txIBLSepcualar.GetDimensions(0, width, height, specularTexureLevels);
        
        float3 PrefilteredColor = txIBLSepcualar.SampleLevel(samLinear, reflect(-Lo, norm), finalRoughness * specularTexureLevels).rgb;
        
        // dot(Normal,View) , roughness를 텍셀좌표로 미리계산된 F*G , G 평균값을 샘플링한다  
        float2 specularBRDF = txIBLLookUpTable.Sample(samLinear, float2(NdotO, finalRoughness)).rg;
        
        // 쿡토런스 Spceular BRDF 근사식
        float3 specularIBL = PrefilteredColor * (F0 * specularBRDF.x + specularBRDF.y); // x : normal dot view, y : roughtness
        
        inDirectLighting = (diffuseIBL + specularIBL); // * AmbientOcclusion;
    }
    
    // return float4(pow(float3(directLighting + inDirectLighting), 1.0 / 2.2), 1.0); // linear -> gamma 
    return float4(directLighting + inDirectLighting, 1.0); // linear -> gamma 
}