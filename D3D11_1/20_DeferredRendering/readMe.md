## 1. 프로젝트 개요

모델의 머티리얼과 기하 정보를 여러 개의 G-Buffer에 먼저 저장하고, 이후 전체 화면 Quad에서 조명을 계산하는 Deferred Rendering 프로젝트입니다.

Geometry Pass에서는 Base Color, Normal, World Position, Metalness, Roughness, Specular, Emission 데이터를 각각의 렌더 타깃에 기록합니다. Directional Light Pass에서는 G-Buffer를 샘플링해 Cook-Torrance PBR, Shadow Mapping, IBL을 계산하고 결과를 백 버퍼에 출력합니다.

## 2. 핵심 기술 포인트

- Multiple Render Targets(MRT)를 이용한 7개 G-Buffer 출력
- Geometry Pass와 Lighting Pass의 분리
- 전체 화면 Quad에서 G-Buffer를 복원하여 PBR 조명 계산
- 방향성 광원의 방향과 색을 별도 상수 버퍼로 전달
- Additive Blend State를 이용한 조명 결과 누적 구조
- G-Buffer의 Normal을 0~1 범위로 인코딩하여 저장하고 조명 패스에서 디코딩
- G-Buffer SRV, Shadow Map, IBL 리소스를 함께 사용한 Lighting Pass
- ImGui에서 각 G-Buffer 내용을 개별 이미지로 확인

## 3. 그래픽스 파이프라인에서의 위치

1. Depth-Only Pass
    - 광원 관점의 깊이를 Shadow Map에 기록합니다.

2. Geometry Pass
    - 7개의 G-Buffer RTV와 Depth Stencil View를 동시에 바인딩합니다.
    - 모델을 한 번 그리면서 Pixel Shader의 `SV_Target0~6`에 표면 정보를 나누어 기록합니다.
    - 이 단계에서는 최종 조명을 계산하지 않습니다.

3. Directional Light Pass
    - 백 버퍼를 Render Target으로 설정하고 G-Buffer를 SRV로 바인딩합니다.
    - 전체 화면 Quad의 각 픽셀에서 G-Buffer 값을 읽어 PBR 직접광과 IBL 간접광을 계산합니다.
    - World Position을 광원 공간으로 변환해 Shadow Map도 샘플링합니다.

4. Skybox Pass
    - 조명 패스 이후 Skybox를 백 버퍼에 렌더링합니다.

## 4. 구현에서 중요한 지점

### G-Buffer 구성

| G-Buffer | 포맷 | 저장 데이터 |
|---|---|---|
| BaseColor | `R8G8B8A8_UNORM_SRGB` | 선형 조명 계산에 사용할 기본 색상 |
| Normal | `R8G8B8A8_UNORM` | 0~1 범위로 인코딩한 월드 공간 법선 |
| WorldPos | `R16G16B16A16_FLOAT` | 월드 공간 위치 |
| Metal | `R8_UNORM` | Metalness |
| Rough | `R8_UNORM` | Roughness |
| Specular | `R8_UNORM` | Specular 강도 |
| Emission | `R8G8B8A8_UNORM` | 자체 발광 색상 |

각 텍스처는 Geometry Pass에서 쓰기 위한 RTV와 Lighting Pass에서 읽기 위한 SRV를 함께 생성합니다.

```cpp
ds.BindFlags = D3D11_BIND_RENDER_TARGET |
               D3D11_BIND_SHADER_RESOURCE;

m_device->CreateTexture2D(&ds, nullptr, &m_gBufferTextures[i]);
m_device->CreateShaderResourceView(
    m_gBufferTextures[i].Get(), nullptr, &m_gBufferSRV[i]);
m_device->CreateRenderTargetView(
    m_gBufferTextures[i].Get(), nullptr, &m_gBufferRTV[i]);
```

### MRT를 이용한 Geometry Pass

```hlsl
struct GBufferOut
{
    float4 Albedo   : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 Position : SV_Target2;
    float  Metal    : SV_Target3;
    float  Rough    : SV_Target4;
    float  Specular : SV_Target5;
    float4 Emission : SV_Target6;
};
```

하나의 Pixel Shader 실행에서 여러 Render Target에 값을 기록합니다. Normal Map은 TBN 행렬로 월드 공간에 옮긴 뒤 `normal * 0.5 + 0.5`로 인코딩합니다.

### 전체 화면 Lighting Pass

```hlsl
float3 baseColor = geoBufferBaseColor.Sample(samPoint, uv).rgb;
float3 normalEncoded = geoBufferNormal.Sample(samPoint, uv).rgb;
float3 worldSpacePos = geoBufferPosition.Sample(samPoint, uv).xyz;
float metal = geoBufferMetal.Sample(samPoint, uv).r;
float rough = geoBufferRough.Sample(samPoint, uv).r;

float3 finalNorm = normalize(DecodeNormal(normalEncoded));
```

화면의 픽셀과 G-Buffer의 픽셀이 일치해야 하므로 G-Buffer에는 Point Sampler를 사용하는 구성이 적합합니다. 복원한 World Position과 Normal을 이용해 카메라 벡터, 광원 벡터, Shadow 좌표를 계산합니다.

### 조명 결과 누적

```cpp
blendDesc.RenderTarget[0].BlendEnable = TRUE;
blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
```

Lighting Pass는 Additive Blend State를 사용합니다. 현재 구현된 조명 패스는 방향성 광원 하나이지만, 같은 방식의 조명 패스를 추가하면 백 버퍼에 결과를 누적할 수 있는 구조입니다.

## 5. 개발 시 주의할 점

1. G-Buffer는 RTV와 SRV로 동시에 바인딩할 수 없습니다.
    - Geometry Pass가 끝나면 G-Buffer RTV를 해제합니다.
    - Lighting Pass가 끝나면 `t11`부터 바인딩한 G-Buffer SRV를 해제합니다.

2. 생성한 G-Buffer 수와 Lighting Pass의 SRV 바인딩 수가 일치해야 합니다.
    - 현재 코드는 7개의 G-Buffer를 생성하지만 Directional Light Pass에서는 Base Color부터 Roughness까지 5개만 `t11~t15`에 바인딩합니다.
    - Pixel Shader는 `t17`의 Specular와 `t18`의 Emission도 샘플링하므로, 두 리소스를 사용하려면 7개 SRV를 모두 바인딩해야 합니다.

3. G-Buffer는 화면 해상도에 비례해 메모리를 사용합니다.
    - 특히 World Position용 `R16G16B16A16_FLOAT` 텍스처는 픽셀당 8바이트를 사용합니다.
    - 화면 크기가 바뀌면 G-Buffer도 새 크기로 다시 생성해야 합니다.

4. Lighting Pass에서는 깊이 쓰기를 끕니다.
    - 전체 화면 Quad가 기존 Geometry Pass의 깊이 값을 변경하지 않도록 `m_depthStencilStateWriteOff`를 사용합니다.

## 6. 실행 결과


## 7. 배운 점

- Deferred Rendering은 모델을 그리는 단계와 조명을 계산하는 단계를 G-Buffer를 기준으로 분리합니다.
- MRT를 사용하면 한 번의 Geometry Pass에서 여러 종류의 표면 데이터를 동시에 저장할 수 있습니다.
- Lighting Pass는 화면 공간에서 실행되므로 장면의 모델 수와 별개로 G-Buffer의 각 픽셀을 기준으로 조명을 계산합니다.
- 같은 텍스처를 패스 사이에서 RTV와 SRV로 전환할 때는 이전 바인딩을 명시적으로 해제해야 합니다.
- G-Buffer의 개수와 포맷은 표현 가능한 머티리얼 정보와 GPU 메모리 사용량 사이의 절충점입니다.
