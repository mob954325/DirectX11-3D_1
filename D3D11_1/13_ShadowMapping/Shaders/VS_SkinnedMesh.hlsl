#include <Shared.fxh>

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    float pos = input.Pos;
    
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
    
    // model -> world space 
    Matrix ModelToWorld = mul(WeightOffsetPose, World);    
    output.Pos = mul(input.Pos, ModelToWorld);
    output.World = output.Pos;    
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) ModelToWorld));
    output.Tangent = normalize(mul(input.Tangent, (float3x3) ModelToWorld));
    output.Bitangent = normalize(mul(input.Bitangent, (float3x3) ModelToWorld));
    
    // texure
    output.Tex = input.Tex;
    
    return output;
} 