//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
TextureCube txCubemap : register(t1); // skybox용 큐브맵 -> 2d 텍스처 배열로 되어있음, 샘플링할 때 float2가 아닌 float3로 받는다.


cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    
    float4 LightDirection;
    float4 LightColor;    
    
    float4 outputColor;    
}

// Basic Shader Struct =============================================================

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD;
}; 

// Functions =============================================================

void ComputeDirectionalLight(float3 lightDirection, float3 normal, // float3 eye,
    out float3 outReflectionVec
)   
{   // 반사 벡터 = -빛 방향 + 2(-빛 방향 dot 노멀)노멀
    // R = -I + 2(-I . N)N;
    
    float3 result = -lightDirection + (2 * (dot(-lightDirection, normal)) * normal);
    outReflectionVec = result;
}
