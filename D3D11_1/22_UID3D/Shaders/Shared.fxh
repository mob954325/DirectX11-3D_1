//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

Texture2D imageTex : register(t0);
SamplerState ObjSamplerState : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

cbuffer imageCB : register(b1)
{
    matrix WVP;
}

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
}; 