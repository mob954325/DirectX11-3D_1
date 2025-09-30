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

struct VS_INPUT_sky
{
    float3 Pos : POSITION;
};

struct PS_INPUT_Sky
{
    float4 positionClipSpace : SV_POSITION;
    float3 position : POSITION;
};