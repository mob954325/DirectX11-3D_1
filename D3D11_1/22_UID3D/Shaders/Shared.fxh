//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

Texture2D imageTex : register(t0);
Texture2D textAtlas : register(t1);
SamplerState ObjSamplerState : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

cbuffer imageCB : register(b1)
{
    matrix imageWVP;
    float4 ImageBaseColor;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct PS_MESH_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};
