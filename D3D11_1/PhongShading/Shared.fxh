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
    
    float4 LightAmbient; // 환경광
    float4 LightDiffuse; // 난반사
    float4 LightSpecular; // 정반사
    
    float4 Indirection; // 간접광
    float Shininess; // 광택지수
    float3 CameraPos; // 카메라 위치
}


//
cbuffer Material : register(b1)
{
    float4 matAmbient;
    float4 matDiffuse;
    float4 matSpecular;  
};


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
    float2 Tex : TEXCOORD0;
    float3 World : TEXCOORD1;
}; 