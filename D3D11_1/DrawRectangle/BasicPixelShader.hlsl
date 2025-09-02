struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    return input.col; // 정점 색상 그대로 출력
}