#include "Shared.hlsli"

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
    float kDir = (roughness + 1) * (roughness + 1); // for directional lighting
    //float kIBL = roughness* roughness / 2.0;          // IBL lighting
    kDir /= 8.0;
    
    return GSchlickGGX(norm, viewVec, kDir) * GSchlickGGX(norm, lightVec, kDir);
}

float4 main(PS_INPUT_QUAD input) : SV_TARGET
{    
    float2 uv = input.tex;
    
    // gbuffer texture sample
    float3 baseColor = geoBufferBaseColor.Sample(samPoint, uv).rgb; // BaseColor
    float3 normalEncoded = geoBufferNormal.Sample(samPoint, uv).rgb; // normalEncoded
    float3 worldSpacePos = geoBufferPosition.Sample(samPoint, uv).xyz; // WS
    float metal = geoBufferMetal.Sample(samPoint, uv).r; // metal cubemap
    float rough = geoBufferRough.Sample(samPoint, uv).r; // rough cubemap    
    float specularIntensity = geoBufferSpecular.Sample(samPoint, uv).r; // specular
    float4 emission = geoBufferEmission.Sample(samPoint, uv); // emission
    
    float3 finalNorm = normalize(DecodeNormal(normalEncoded));
    
    float3 lightDirWorld = normalize(-LightDirection.xyz);
    float intensity = LightDirection.w;
    
    float3 lightColor = LightColor.rgb;
    
    float3 Lo = normalize((float3) CameraPos - (float3) worldSpacePos); // 빛이 눈으로 가는 방향 : 현재 위치 -> eye ( view Vector )

    float3 Li = normalize(lightDirWorld); // 빛 방향
    float3 Lh = normalize(Li + Lo); // half-vector between Li and Lo        
    
    float NdotL = saturate(dot(finalNorm, Li)); // dot(Normal, Light Direction)
    float NdotO = saturate(dot(finalNorm, Lo)); // dot(Normal, View)
    
    // 그림자처리 부분 =====================================================================
    float finalShadow = 1.0f;
    // Light Space
    float4 positionShadow = mul(float4(worldSpacePos, 1.0f), ShadowView);
    positionShadow = mul(positionShadow, ShadowProjection);

    // NDC depth
    float currentShadowDepth = positionShadow.z / positionShadow.w;

    // Shadow UV
    float2 shadowUV = positionShadow.xy / positionShadow.w;
    shadowUV.y = -shadowUV.y;
    shadowUV = shadowUV * 0.5f + 0.5f;
    
    // NDC좌표계에서 Texture 좌표계로 변환
    if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 && shadowUV.y >= 0.0 && shadowUV.y <= 1.0)
    {
        float sampleShadowDepth = txShadow.Sample(samLinear, shadowUV).r;
        
        // currentShadowDepth가 더 크면 뒤 쪽에 있으므로 직접광 차단
        if (currentShadowDepth > sampleShadowDepth + 0.001)
        {
            finalShadow = 0.0f;
        }
    }    
    
    // =============================================
    
    float3 directLighting = 0.0f;
    
    // Cook-Torrance Specular BRDF   

    
    // 기본 반사율(F0) = lerp(비금속 평균 반사, baseColor(텍스처), matalness)
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), (float3) baseColor, metal);
    
    {   
        float D = NDFGGXTR(finalNorm, Lh, max(0.001, rough));
        float3 F = FresnelSchlick(F0, max(0, dot(Lh, Lo)));
        float G = GSmithMethod(finalNorm, Lo, Li, rough);
    
        // 표면 산란
        float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metal);
    
        // Lambert diffuse BRDF  
        float3 diffuseBRDF = kd * (float3) baseColor / PI;
    
        // specular BRDF
        float3 specularBRDF = (D * F * G) / max(Epsilon, 4.0 * NdotL * NdotO);
        
        directLighting = (diffuseBRDF + specularBRDF * specularIntensity) * NdotL * finalShadow * lightColor * intensity; + (float3) emission;
    }
    
    
   float3 inDirectLighting = 0.0f;
   // IBL
   {
        float3 irradiance = txIBLIrradiance.Sample(samLinear, finalNorm).rgb;
        
        // IBL diffuse    
        float3 F = FresnelSchlick(F0, NdotO);
        float3 kd = lerp(1.0 - F, 0.0, metal);
        
        float3 diffuseIBL = kd * (float3)baseColor * irradiance / PI; // irradiance map이 1/pi가 포함되어있으면 제거 있으면 1/pi 추가하기
        
        // IBL specular        
        uint specularTexureLevels, width, height;        
        txIBLSepcualar.GetDimensions(0, width, height, specularTexureLevels);       
        
        float3 R = reflect(-Lo, finalNorm);
        
        float3 PrefilteredColor = txIBLSepcualar.SampleLevel(samLinear, R, rough * specularTexureLevels).rgb;
        
        // dot(Normal,View) , roughness를 텍셀좌표로 미리계산된 F*G , G 평균값을 샘플링한다  
        float2 specularBRDF = txIBLLookUpTable.Sample(samLinear, float2(NdotO, rough)).rg;
        
        // 쿡토런스 Spceular BRDF 근사식
        float3 specularIBL = PrefilteredColor * (F0 * specularBRDF.x + specularBRDF.y); // x : normal dot view, y : roughtness
        
        inDirectLighting = (diffuseIBL + specularIBL);
   }
    
    return float4(pow(float3(directLighting + inDirectLighting), 1.0 / 2.2), 1.0); // linear -> gamma    
}