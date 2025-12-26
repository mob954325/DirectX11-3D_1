//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

SamplerState samLinear      : register(s0);
SamplerState samPoint       : register(s1);

Texture2D txDiffuse         : register(t0);   // 
Texture2D txEmission        : register(t1);   // Emission 
Texture2D txNormal          : register(t2);   // 노멀맵 텍스처
Texture2D txSpec            : register(t3);   // 스펙큘러맵 텍스처
Texture2D txShadow          : register(t4);   // 그림자 매핑 텍스처
TextureCube txCubemap       : register(t5);   // skybox용 큐브맵 텍스처

Texture2D txMetalness       : register(t6);   // metalness 텍스처
Texture2D txRoughness       : register(t7);   // roughness 텍스처

TextureCube txIBLIrradiance : register(t8);   // IBL Irrandiance Map
TextureCube txIBLSepcualar  : register(t9);   // IBL Specular Map
Texture2D txIBLLookUpTable  : register(t10);  // IBL LUT (Look Up Table)

Texture2D geoBufferBaseColor: register(t11);  // gbuffer basecolor texture
Texture2D geoBufferNormal   : register(t12);  // gbuffer normal texture
Texture2D geoBufferPosition : register(t13);  // gbuffer worldSpace position texture

cbuffer ConstantBuffer : register(b0)   // PerFrame
{
    matrix View;
    matrix Projection;
    
    matrix ShadowView;
    matrix ShadowProjection;    
    
    float4 LightAmbient;    // 환경광
    float4 LightDiffuse;    // 난반사
    float4 LightSpecular;   // 정반사    
    float Shininess;        // 광택지수
    float3 CameraPos;       // 카메라 위치        
    
    float Metalness;
    float Roughness;
    float2 pad;
}

cbuffer Material : register(b1) // PerMaterial
{
    float4 matAmbient;
    float4 matDiffuse;
    float4 matSpecular;  
    
    int hasDiffuse;
    int hasEmissive;
    int hasNormal;
    int hasSpecular;
    
    int hasMetalness;
    int hasRoughness;    
    int hasShininess;    
    float pad2;
};

cbuffer ModelTransform : register(b2)
{
    matrix World;        
    
    int isRigid; // 1 : rigid, 0 : skinned
    int refBoneIndex; // 리지드일 때 참조하는 본 인덱스
    float pad3;
    float pad4;
}

cbuffer BonePoseMatrix : register(b3) 
{
    matrix bonePose[256]; // skeleton pose matrix -> animation updated matrix
}

cbuffer BoneOffsetMatrix : register(b4)
{
    matrix boneOffset[256]; // model transform
}

cbuffer DirectionalLightCB : register(b5)
{
    float4 LightDirection;
    float4 LightColor; 
}

// Basic Shader Struct =============================================================

struct VS_INPUT_MODEL
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;   
    
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;    
    float3 Norm : NORMAL;
    
    int4 BlendIndices : BLENDINDICES;
    float4 BlendWeights : BLENDWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;   // 투영한 위치 좌표
    
    float3 Norm : NORMAL;       // 노멀 값
    float2 Tex : TEXCOORD;      // 텍스처 UV좌표
    float3 World : TEXCOORD2;   // 월드 좌표
    
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;
    
    float4 PositionShadow : TEXCOORD3; // 그림자 위치
}; 

struct VS_INPUT_sky
{
    float3 Pos : POSITION;
};

struct PS_INPUT_Sky
{
    float4 PosClipSpace : SV_POSITION;
    float3 Pos : POSITION;
};

struct VS_INPUT_QUAD
{
    float3 position : POSITION; // ndc
    float2 tex : TEXCOORD;
};

struct PS_INPUT_QUAD
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
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

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}