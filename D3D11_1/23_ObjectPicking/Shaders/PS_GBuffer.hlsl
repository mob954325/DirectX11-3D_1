#include <Shared.hlsli>

struct GBufferOut
{
    float4 Albedo   : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 Position : SV_Target2;
    float1 Metal    : SV_Target3;
    float1 Rough    : SV_Target4;
    float1 Specular : SV_Target5;
    float4 Emission : SV_Target6;
    uint  PickingID: SV_Target7;
};

GBufferOut main(PS_INPUT_MODEL input)
{
    GBufferOut gOut;
    gOut.Position = float4(input.PosWS, 1.0f);
    
    // 광원처리 부분 =====================================================================
    // base(diffuse) texture Sampling 
    float4 albedo = txDiffuse.Sample(samLinear, input.Tex);
    if (!hasDiffuse)
    {
        albedo = BaseColor;
    }
    
    if (albedo.a < 0.5f) // alpha cliping
        discard;
    
    albedo.rgb = pow(albedo.rgb, 2.2); // gamma -> linear
    gOut.Albedo = albedo;
    
    // specularSample
    float specularIntensity = txSpec.Sample(samLinear, input.Tex).r;
    if (!hasSpecular)
    {
        specularIntensity = 1.0f;
    }
    gOut.Specular = specularIntensity;
    
    // EmissionSample
    float4 textureEmission = txEmission.Sample(samLinear, input.Tex);
    if (!hasEmissive)
    {
        textureEmission = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    gOut.Emission = textureEmission;
    
    // normalSample
    float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.NormWS);
    float3 normalMapSample = txNormal.Sample(samLinear, input.Tex).rgb;
    // normal map이 없을 경우를 대비
    if (!hasNormal)
    {
        normalMapSample = float3(0.5f, 0.5f, 1.0f); // flat normal (no perturbation)
    }
    float3 normalTexture = normalize(DecodeNormal(normalMapSample)); //  Convert normal map color (RGB [0,1]) to normal vector in range [-1,1] 
    float3 TBNSpace = normalize(mul(normalTexture, TBN));  
    float3 finalNorm = float4(EncodeNormal(TBNSpace), 1.0f);
    gOut.Normal = float4(finalNorm, 1.0f);
    
    
    // MetalnessSample
    float metalnessSample = txMetalness.Sample(samLinear, input.Tex).r; // grayScale
    if(!hasMetalness)
    {
        metalnessSample = 1;
    }
    
    float finalMetalness = metalnessSample * Metalness;
    gOut.Metal = finalMetalness;
    
    // RoughnessSample
    float roughnessSample = txRoughness.Sample(samLinear, input.Tex).r; // grayScale
    if(!hasRoughness)
    {
        roughnessSample = hasShininess ? 1 - roughnessSample : 1;
    }
    
    float finalRoughness = roughnessSample * Roughness;
    gOut.Rough = finalRoughness;

    gOut.PickingID = PickingID;
    
    return gOut;    
}