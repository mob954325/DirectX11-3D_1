//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

SamplerState samLinear : register(s0);

Texture2D txDiffuse : register(t0);     // 
Texture2D txEmission : register(t1);    // Emission 
Texture2D txNormal : register(t2);      // 노멀맵 텍스처
Texture2D txSpec : register(t3);        // 스펙큘러맵 텍스처
// TextureCube txCubemap : register(t1); // skybox용 큐브맵 텍스처

cbuffer ConstantBuffer : register(b0) // PerFrame
{
    matrix View;
    matrix Projection;
    
    float4 LightDirection;
    float4 LightColor;        
    
    float4 LightAmbient; // 환경광
    float4 LightDiffuse; // 난반사
    float4 LightSpecular; // 정반사
    
    float Shininess; // 광택지수
    float3 CameraPos; // 카메라 위치        
}

cbuffer Material : register(b1) // PerMaterial
{
    float4 matAmbient;
    float4 matDiffuse;
    float4 matSpecular;  
    
    bool hasDiffuse;
    bool hasEmissive;
    bool hasNormal;
    bool hasSpecular;
};

cbuffer ModelTransform : register(b2)
{
    matrix World;        
    
    int isRigid; // 1 : rigid, 0 : skinned
    int refBoneIndex; // 리지드일 때 참조하는 본 인덱스
    float pad1;
    float pad2;
}

cbuffer ModelMatrix : register(b3) // Skinning
{
    matrix modelMatricies[32]; // model space matricies
}


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
    float2 Tex : TEXCOORD;      // 텍스처 UV좌표
    float3 World : TEXCOORD2;   // 월드 좌표
    
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