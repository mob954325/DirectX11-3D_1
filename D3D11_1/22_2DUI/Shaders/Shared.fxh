//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

cbuffer PerObjCB : register(b1)
{
    matrix WVP;
}

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
}; 