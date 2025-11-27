## 1. 프로젝트 개요

환경 렌더링의 핵심인 Skybox를 추가합니다.

Skybox는 무한히 먼 거리의 배경(하늘, 우주 등)을 표현하는 기법으로,
Cube Map 텍스처를 사용해 카메라를 감싸는 거대한 큐브를 렌더링합니다.

## 2. 핵심 기술 포인트

- Cube Map 텍스처 활용
    - 6개 면(+X, -X, +Y, -Y, +Z, -Z)으로 구성된 큐브 텍스처
    - 3D 방향 벡터로 샘플링 (UV 좌표 대신)
    - DDS 포맷의 큐브맵 로드 (CreateDDSTextureFromFile)
- View 행렬 Translation 제거
    - 카메라가 이동해도 스카이박스는 항상 카메라를 중심으로
    - VS에서 View 행렬의 4행 4열(이동 성분)을 제거
    - 결과: 카메라가 "절대 닿을 수 없는" 무한 거리 배경 표현
- Depth Test 비활성화
    - DepthEnable = FALSE, DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO
    - 모든 오브젝트보다 뒤에 렌더링
    - 깊이 버퍼에 값을 쓰지 않음 (다른 오브젝트 깊이 보존)
- Rasterizer State 설정
    - 스카이박스: CullMode = D3D11_CULL_BACK (큐브 내부만 보임)
    - 일반 오브젝트: CullMode = D3D11_CULL_FRONT
    - FrontCounterClockwise = TRUE로 와인딩 순서 정의
- 별도 렌더링 패스 구성
    - InitSkyBox()로 스카이박스 전용 리소스 생성
    - OnRender()에서 스카이박스 → 일반 오브젝트 순서로 렌더링
    - 각 패스마다 Vertex Buffer, Shader, State 교체
- TextureCube 샘플링
    - HLSL: TextureCube 타입 사용
    - 정점 위치(방향 벡터)로 직접 샘플링
    - 자동으로 6개 면 중 적절한 텍스처 선택

 

## 3. 그래픽스 파이프라인에서의 위치

- Input Assembler:
CubeVertex 구조체 (Vector3 position만 사용)
정점 24개 (큐브 6면 × 4정점)
Input Layout: POSITION만 정의  

- Vertex Shader (SkyboxVertexShader):
    1. View 행렬의 Translation 제거
    2. MVP 변환 수행 (이동 없는 View 사용)
    3. 정점 위치를 PS로 전달 (이것이 방향 벡터 역할)  

- Rasterizer:
CullMode = D3D11_CULL_BACK
→ 큐브를 뒤집어서 내부만 보이게 (카메라가 큐브 중심)
FillMode = D3D11_FILL_SOLID
FrontCounterClockwise = TRUE
- Pixel Shader (SkyboxPixelShader):
    1. 정점 위치를 3D 방향 벡터로 사용해 큐브맵 샘플링  

- Output Merger:
DepthStencilState: DepthEnable = FALSE
→ 깊이 테스트 하지 않음 (항상 통과)
DepthWriteMask = ZERO
→ 깊이 버퍼에 값을 쓰지 않음
결과: 모든 오브젝트 뒤에 그려짐

## 4. 구현에서 중요한 지점

1. View 행렬 Translation 제거

```
 Matrix ViewWithoutTranlation = View;
    
 // 이동 성분 제거
 ViewWithoutTranlation._41 = 0;
 ViewWithoutTranlation._42 = 0;
 ViewWithoutTranlation._43 = 0;
```

- 왜?: 카메라가 이동해도 스카이박스는 항상 카메라 중심
- 효과: "무한히 먼 배경" 느낌 (절대 닿을 수 없음)
- 회전은 유지: 카메라 방향에 따라 보이는 하늘 부분 변경

2. Depth Stencil State 설정

```cpp
   D3D11_DEPTH_STENCIL_DESC skyboxDsDesc = {};
   skyboxDsDesc.DepthEnable = FALSE;               // 깊이 테스트 비활성화
   skyboxDsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // 깊이 쓰기 금지
   skyboxDsDesc.StencilEnable = FALSE;
   CreateDepthStencilState(&skyboxDsDesc, &m_pSkyDepthStencilState);

   // 렌더링 시
   m_pDeviceContext->OMSetDepthStencilState(m_pSkyDepthStencilState, 1);

```

- 왜 DepthEnable = FALSE?
→ 스카이박스는 "항상 뒤"에 있어야 함
→ 깊이 비교 없이 무조건 렌더링

- 왜 DepthWriteMask = ZERO?
→ 일반 오브젝트의 깊이 정보를 보존
→ 스카이박스 이후 렌더링되는 오브젝트의 깊이 테스트 보장

대안: DepthFunc = D3D11_COMPARISON_LESS_EQUAL

- Z = 1.0으로 고정하고 LESS_EQUAL 비교
- 모든 오브젝트(Z < 1.0) 통과 후 스카이박스 렌더링

3. Rasterizer State - Backface Culling

```cpp
   // 스카이박스용
   D3D11_RASTERIZER_DESC skyRasterizerDesc = {};
   skyRasterizerDesc.CullMode = D3D11_CULL_BACK;
   skyRasterizerDesc.FillMode = D3D11_FILL_SOLID;
   skyRasterizerDesc.FrontCounterClockwise = TRUE;
   CreateRasterizerState(&skyRasterizerDesc, &m_pSkyRasterizerState);

   // 일반 오브젝트용
   D3D11_RASTERIZER_DESC objRasterizerDesc = {};
   objRasterizerDesc.CullMode = D3D11_CULL_FRONT;  // 주의!

```

- 왜 스카이박스는 CULL_BACK?
→ 큐브의 "내부"만 보여야 함 (카메라가 큐브 중심)
→ 바깥면(앞면)은 컬링

- 왜 일반 오브젝트는 CULL_FRONT?
→ 코드에서 FrontCounterClockwise = TRUE
→ CCW 와인딩이 Front면이 되므로, 실제로는 CW면을 컬링
→ 일반적인 D3D11 관례와 일치

4. Cube Map 샘플링

```
   // SkyboxPixelShader.hlsl
   TextureCube g_CubeMap : register(t1);  // 주의: t1!
   SamplerState g_Sampler : register(s0);

   float4 main(VS_OUTPUT input) : SV_TARGET {
       float3 direction = normalize(input.position);
       return g_CubeMap.Sample(g_Sampler, direction);
   }

```

- 왜 register(t1)?
→ register(t0)는 일반 오브젝트 텍스처 사용 중
→ 스카이박스 전용 슬롯 분리

- 왜 정점 위치를 방향 벡터로?
→ 큐브맵은 3D 방향으로 샘플링
→ GPU가 자동으로 6개 면 중 적절한 면 선택

5. 렌더링 순서

```cpp
   // 1. 스카이박스 렌더링
   OMSetDepthStencilState(skyDepthStencil);
   RSSetState(skyRasterizer);
   PSSetShaderResources(1, 1, &cubeMapSRV);  // t1
   DrawIndexed(skyboxIndices);

   // 2. 일반 오브젝트 렌더링
   OMSetDepthStencilState(normalDepthStencil);
   RSSetState(normalRasterizer);
   PSSetShaderResources(0, 1, &normalTextureSRV);  // t0
   DrawIndexed(objectIndices);

```

- 왜 스카이박스를 먼저?
→ DepthEnable = FALSE라 순서 무관하지만
→ Early-Z 최적화 방지 (픽셀 전부 처리)

6. Near Plane 설정 차이

```cpp
   // 스카이박스용 Projection
   Matrix m_skyboxProjection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, 0.1, m_Far);

   // 일반 오브젝트용 Projection
   Matrix m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);
```

- 왜 Near가 다른가?
→ 스카이박스는 정점이 카메라 매우 가까이 (큐브 중심)
→ Near = 0.01이면 클리핑될 수 있음
→ Near = 0.1로 여유있게 설정

## 5. 개발 과정에서 겪은 문제 & 해결 

1. 스카이박스가 일반 오브젝트를 가림
    - 문제:
    스카이박스를 렌더링하면 큐브 오브젝트가 보이지 않음
    스카이박스 픽셀이 모든 오브젝트 위에 그려짐
    - 원인:
    스카이박스 렌더링 시 Depth Test가 활성화되어 있었음
    스카이박스 정점이 카메라에 가까워(Near Plane 근처)
    깊이 값이 작아서 일반 오브젝트보다 "앞"에 배치됨
    - 해결:
    Depth Stencil State를 별도로 생성
        
        ```cpp
          skyboxDsDesc.DepthEnable = FALSE;
          skyboxDsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        ```
        
        렌더링 전 OMSetDepthStencilState()로 교체
        
    
2. 카메라 이동 시 스카이박스가 따라옴
    - 문제:
    초기에는 Translation 제거 없이 구현
    카메라가 이동하면 스카이박스도 함께 이동
    스카이박스 경계가 보이며 "무한 배경" 느낌 상실
    - 원인:
    일반적인 View 행렬을 그대로 사용
    카메라 이동이 스카이박스에도 적용됨
    - 해결:
    Vertex Shader에서 View 행렬의 Translation 제거
        
        ```
          float4x4 viewNoTranslation = View;
          viewNoTranslation._41_42_43 = float3(0, 0, 0);
        ```
        
    - 추가로 이해한 점:
    행렬의 4행(row-major) / 4열(column-major)이 이동 성분
    회전 성분은 유지해야 카메라 방향에 따라 하늘 변경

## 6. 실행 결과

## 7. 배운 점

- Cube Map 텍스처의 특성
    - 6개 면으로 구성된 특수 텍스처 타입
    - UV 좌표 대신 3D 방향 벡터로 샘플링
    - 환경 맵, 리플렉션, 굴절 등 다양한 용도
- 무한 거리 배경 구현 원리
    - View 행렬에서 Translation 제거
    - 카메라는 항상 "세계의 중심"
    - 회전은 유지 → 방향에 따른 환경 변화
- 렌더링 상태 관리의 중요성
    - Depth Stencil State: 깊이 테스트 제어
    - Rasterizer State: 컬링 모드, 와인딩 순서
    - 텍스처 슬롯 분리: 여러 텍스처 동시 사용
    - 각 렌더링 패스마다 적절한 상태로 교체 필수
- 렌더링 순서 최적화
    - 불투명 오브젝트: 앞→뒤 (Early-Z)
    - 스카이박스: 마지막 (깊이 테스트 없음)
    - 투명 오브젝트: 뒤→앞 (Alpha Blending)
- Depth Buffer 쓰기 제어
    - DepthWriteMask로 깊이 쓰기 방지 가능
    - 이후 렌더링 오브젝트의 깊이 정보 보존