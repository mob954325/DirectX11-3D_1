#include <Shared.fxh>

static const float PI = 3.14159265359f;
static const float TWO_PI = 6.28318530718f;
static const float EPS = 1e-6f;

// 1D 9-slice remap (t: 0..1 output -> return 0..1 source)
float Remap9_1D(float t, float outL, float outR, float srcL, float srcR)
{
    // center가 존재하는 일반 케이스
    float outC = 1.0f - outL - outR;
    float srcC = 1.0f - srcL - srcR;

    // 너무 작아서 center가 음수/0이면(=rect가 border보다 작음) 좌/우만으로 분배
    if (outC <= EPS)
    {
        float sum = max(outL + outR, EPS);
        float split = outL / sum; // 좌 border 비율
        if (t < split)
        {
            float tt = t / max(split, EPS);
            return lerp(0.0f, srcL, tt);
        }
        else
        {
            float tt = (t - split) / max(1.0f - split, EPS);
            return lerp(1.0f - srcR, 1.0f, tt);
        }
    }

    if (t < outL) // 현재 텍셀이 왼쪽 보더 안에 있으면
    {
        float tt = t / max(outL, EPS); // 왼쪽 보더를 최대값으로 매핑할 위치 찾기
        return lerp(0.0f, srcL, tt);
    }
    else if (t > 1.0f - outR) // 오른쪽 보더 안에 있으면
    {
        float tt = (t - (1.0f - outR)) / max(outR, EPS); // 오른쪽 보더 기준으로 위치 찾기
        return lerp(1.0f - srcR, 1.0f, tt);
    }
    else // 센터에 있으면
    {
        float tt = (t - outL) / max(outC, EPS); // 중앙값을 기준으로 위치 찾기
        return srcL + tt * srcC;
    }
}

float4 main(PS_INPUT input) : SV_TARGET
{
    // type: 0=Simple, 1=Sliced, 2=Fill
    int type = (int) (imageParams.x + 0.5f);
    float2 uv = input.TexCoord;

    // --- SLICED (9-slice) ---
    if (type == 1)
    {
        // border px
        float leftPx = UVRect.x;
        float rightPx = UVRect.y;
        float topPx = UVRect.z;
        float bottomPx = UVRect.w;

        // sizes
        float rectW = max(imageSize.x, 1.0f);   // 이미지 rect width 값
        float rectH = max(imageSize.y, 1.0f);   // 이미지 rect height 값
        float texW = max(imageSize.z, 1.0f);    // 이미지 width 픽셀 값
        float texH = max(imageSize.w, 1.0f);    // 이미지 height 픽셀 값

        // borderPx값을 0 ~ 1 사이 값으로 변환 -> 각 보더가 전체 폭의 %인가? ( 스크린에 그려질 분할선 위치 )
        float outL = saturate(leftPx / rectW);
        float outR = saturate(rightPx / rectW);
        float outT = saturate(topPx / rectH);
        float outB = saturate(bottomPx / rectH);

        // 이미지 픽셀 기준 값 변환
        float srcL = saturate(leftPx / texW);
        float srcR = saturate(rightPx / texW);
        float srcT = saturate(topPx / texH);
        float srcB = saturate(bottomPx / texH);

        // 좌상단이 0,0일 때 처리
        float u2 = Remap9_1D(uv.x, outL, outR, srcL, srcR);
        float v2 = Remap9_1D(uv.y, outT, outB, srcT, srcB);

        uv = float2(u2, v2);
    }
    
    // --- FILL (Radial CW) ---
    if (type == 2)
    {
        float fillAmount = saturate(imageParams.y); // 0 - 1 범위 강제

        // uv 기준 중심
        float2 uv = input.TexCoord;
        float2 c = float2(0.5f, 0.5f);
        float2 v = uv - c;

        // 중심점은 항상 통과(각도 불능 영역)
        float r2 = dot(v, v);
        if (r2 > 1e-8f)
        {
            // atan2: -pi..pi, 0은 +x(3시), CCW +
            float ang = atan2(-v.y, v.x);

            // "12시 시작, 시계방향 증가"로 변환
            // top(12시)은 ang=+pi/2 -> 0이 되게: (pi/2 - ang)
            float cw = (1.57079632679f - ang);

            // 0..2pi 로 래핑
            const float TWO_PI = 6.28318530718f;
            cw = fmod(cw, TWO_PI);
            if (cw < 0)
                cw += TWO_PI;

            float t = cw / TWO_PI; // 0..1

            // 채워진 영역만 통과
            clip(fillAmount - t);
        }
    }    
    
    float4 sampleTex = imageTex.Sample(ObjSamplerState, uv);
    return sampleTex * ImageBaseColor;
}