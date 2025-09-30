//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
// TextureCube txCubemap : register(t1); // skybox용 큐브맵 -> 2d 텍스처 배열로 되어있음, 샘플링할 때 float2가 아닌 float3로 받는다.
Texture2D txNormal : register(t1); // 노멀맵 텍스처
Texture2D txSpec : register(t2);    // 스펙큘러맵 텍스처

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
    float4 Pos : POSITION0;
    float2 Tex : TEXCOORD;    
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;    
    float3 Norm : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;   // 투영한 위치 좌표
    float3 Norm : NORMAL;       // 노멀 값
    float2 Tex : TEXCOORD0;     // 텍스처 UV좌표
    float3 World : TEXCOORD1;   // 월드 좌표
    
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;
}; 

// Functions =======================================================================
float3 EncodeNormal(float3 N)
{
    return N * 0.5 + 0.5;
}

float3 DecodeNormal(float3 N)
{
    return N * 2 - 1;
}