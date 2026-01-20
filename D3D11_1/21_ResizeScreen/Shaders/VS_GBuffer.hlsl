#include <Shared.hlsli>

PS_INPUT_MODEL main(VS_INPUT_MODEL input)
{
    PS_INPUT_MODEL output = (PS_INPUT_MODEL) 0;
    
    Matrix ModelToWorld;
    if (isRigid == 0)
    {
        // offsetMatrix * poseMatrix
        float4x4 OffsetPose[4];
        OffsetPose[0] = mul(boneOffset[input.BlendIndices.x], bonePose[input.BlendIndices.x]);
        OffsetPose[1] = mul(boneOffset[input.BlendIndices.y], bonePose[input.BlendIndices.y]);
        OffsetPose[2] = mul(boneOffset[input.BlendIndices.z], bonePose[input.BlendIndices.z]);
        OffsetPose[3] = mul(boneOffset[input.BlendIndices.w], bonePose[input.BlendIndices.w]);
    
        // 4개를 가중치 누적 합으로 변경
        float4x4 WeightOffsetPose;
        WeightOffsetPose = mul(input.BlendWeights.x, OffsetPose[0]);
        WeightOffsetPose += mul(input.BlendWeights.y, OffsetPose[1]);
        WeightOffsetPose += mul(input.BlendWeights.z, OffsetPose[2]);
        WeightOffsetPose += mul(input.BlendWeights.w, OffsetPose[3]);
        
        ModelToWorld = mul(WeightOffsetPose, World);
    }
    else if (isRigid == 1)
    {
        ModelToWorld = mul(bonePose[refBoneIndex], World); 
        // ModelToWorld = World;
    }
    
    // model -> world space    
    float4 worldPos = mul(input.Pos, ModelToWorld);
    output.PosWS = worldPos.xyz;
    float4 PosVS = mul(worldPos, View); // view space
    output.PosCS = mul(PosVS, Projection);

    output.NormWS = normalize(mul(input.Norm, (float3x3) ModelToWorld));
    output.Tangent = normalize(mul(input.Tangent, (float3x3) ModelToWorld));
    output.Bitangent = normalize(mul(input.Bitangent, (float3x3) ModelToWorld));

    output.Tex = input.Tex;
    
    return output;
} 