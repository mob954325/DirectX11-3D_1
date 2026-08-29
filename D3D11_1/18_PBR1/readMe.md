## 1. 프로젝트 개요

기존의 Phong 계열 조명 대신 물리 기반 렌더링(PBR)을 적용하는 프로젝트입니다.

픽셀 셰이더에서 Cook-Torrance BRDF를 계산하고, FBX 머티리얼에서 불러온 Base Color, Normal, Metalness, Roughness, Ambient Occlusion 텍스처를 조명 계산에 사용합니다. 또한 Irradiance Map, Prefiltered Specular Map, BRDF LUT를 이용한 Image Based Lighting(IBL)을 더해 직접광이 닿지 않는 영역에도 환경광이 반영되도록 구성했습니다.

## 2. 핵심 기술 포인트

- Cook-Torrance BRDF를 이용한 직접광 계산
    - GGX/Trowbridge-Reitz Normal Distribution Function
    - Fresnel-Schlick 근사
    - Smith 기법을 이용한 Geometry 항
- Metalness와 Roughness에 따른 Diffuse/Specular 반사 비율 조절
- Irradiance Cube Map을 이용한 간접 난반사
- Prefiltered Specular Cube Map과 BRDF LUT를 이용한 간접 정반사
- Base Color 텍스처를 감마 공간에서 선형 공간으로 변환한 뒤 조명 계산
- 기존 Shadow Mapping 결과를 직접광 항에 적용

## 3. 그래픽스 파이프라인에서의 위치

1. CPU 및 리소스 로딩 단계
    - `FBXResourceManager`가 FBX 머티리얼에서 Diffuse, Emissive, Normal, Specular, Metalness, Roughness, Shininess, Ambient Occlusion 텍스처를 구분하여 로드합니다.
    - 환경 조명을 위해 Irradiance Map, Prefiltered Specular Map, BRDF LUT를 DDS 리소스로 로드합니다.

2. Vertex Shader 단계
    - Rigid/Skinned Mesh의 정점을 월드 공간으로 변환합니다.
    - 월드 위치, 법선, 탄젠트, 바이탄젠트와 Shadow Map 샘플링에 필요한 광원 공간 위치를 Pixel Shader로 전달합니다.

3. Pixel Shader 단계
    - 머티리얼 텍스처를 샘플링하고 TBN 행렬로 Normal Map의 법선을 월드 공간으로 변환합니다.
    - 직접광에는 Cook-Torrance BRDF와 Shadow Map 결과를 적용합니다.
    - 간접광에는 IBL Diffuse와 IBL Specular 결과를 적용합니다.

4. Output-Merger 단계
    - PBR 조명 결과를 백 버퍼에 기록하고 Skybox와 함께 출력합니다.

## 4. 구현에서 중요한 지점

### Cook-Torrance BRDF 구성

정반사 항은 NDF, Fresnel, Geometry 항을 결합하여 계산합니다.

```hlsl
float D = NDFGGXTR(norm, Lh, max(0.001, finalRoughness));
float3 F = FresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
float G = GSmithMethod(norm, Lo, Li, finalRoughness);

float3 specularBRDF = (D * F * G)
    / max(Epsilon, 4.0 * NdotL * NdotO);
```

- `D`: 미세면 법선이 Half Vector 방향으로 분포할 확률
- `F`: 시선 각도에 따라 달라지는 반사율
- `G`: 미세면 사이의 가림과 그림자 효과

0으로 나누는 상황을 피하기 위해 분모에 `Epsilon`을 적용하고, Roughness에도 최소값을 둡니다.

### 금속과 비금속의 반사율 구분

```hlsl
float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, finalMetalness);
float3 kd = lerp(1.0 - F, 0.0, finalMetalness);
float3 diffuseBRDF = kd * albedo.rgb / PI;
```

비금속은 기본 반사율 `0.04`를 사용하고, 금속에 가까워질수록 Base Color를 정반사 색으로 사용합니다. 금속 표면은 난반사 비율이 줄어들도록 `kd`를 계산합니다.

### IBL을 이용한 간접광

```hlsl
float3 irradiance = txIBLIrradiance.Sample(samLinear, norm).rgb;
float3 diffuseIBL = kd * albedo.rgb * irradiance / PI;

float3 R = reflect(-Lo, finalNorm);
float lodLevel = finalRoughness * specularTexureLevels - 1;
float3 prefilteredColor =
    txIBLSepcualar.SampleLevel(samLinear, R, lodLevel).rgb;

float2 brdf = txIBLLookUpTable.Sample(
    samLinear, float2(NdotO, finalRoughness)).rg;
float3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);
```

Roughness가 높을수록 Prefiltered Specular Map의 높은 Mip Level을 읽어 흐린 반사를 사용합니다. BRDF LUT에는 `NdotV`와 Roughness에 따른 BRDF 적분 결과가 저장되어 있습니다.

### 텍스처 슬롯 구성

- `t0~t3`: Diffuse, Emission, Normal, Specular
- `t4`: Shadow Map
- `t5`: Skybox Cube Map
- `t6~t7`: Metalness, Roughness
- `t8~t10`: Irradiance, Prefiltered Specular, BRDF LUT
- `t11`: Ambient Occlusion

## 5. 개발 시 주의할 점

1. 색 공간을 일관되게 유지해야 합니다.
    - Base Color는 `pow(color, 2.2)`로 선형 공간으로 변환한 뒤 조명 계산에 사용합니다.
    - 최종 결과는 `pow(color, 1.0 / 2.2)`로 감마 보정하여 출력합니다.

2. 머티리얼에 텍스처가 없는 경우를 처리해야 합니다.
    - `hasDiffuse`, `hasNormal`, `hasMetalness`, `hasRoughness` 등의 플래그를 확인해 기본값을 사용합니다.
    - 기본값이 실제 머티리얼의 의도와 다르면 금속성이나 표면 거칠기가 예상과 다르게 보일 수 있습니다.

3. Roughness로 계산한 Mip Level은 유효 범위 안에 있어야 합니다.
    - 현재 코드는 `roughness * mipLevels - 1`을 사용하므로 Roughness가 0에 가까우면 음수 LOD가 계산될 수 있습니다.
    - 필요하면 `clamp`를 사용해 `0 ~ mipLevels - 1` 범위로 제한할 수 있습니다.

## 6. 실행 결과
Roughness  

https://github.com/user-attachments/assets/14992a8b-d412-48c4-92ed-b88e0403856a  

Metalness  

https://github.com/user-attachments/assets/2eaad5c3-47ff-4409-b27f-73fff827f6db  

AmbientOcclusion  

https://github.com/user-attachments/assets/a50ff433-db9c-4c57-a0c6-d66b0873750b  

## 7. 배운 점

- PBR은 Diffuse와 Specular를 별도로 더하는 것뿐 아니라 에너지 보존을 고려해 두 반사 비율을 함께 조절해야 합니다.
- Metalness는 기본 반사율과 난반사 비율을 결정하고, Roughness는 정반사의 크기와 선명도에 영향을 줍니다.
- IBL은 Cube Map 한 장을 그대로 사용하는 것이 아니라 Diffuse, Specular, BRDF 적분 결과를 용도별 리소스로 나누어 사용할 수 있습니다.
- 조명 계산은 선형 색 공간에서 수행하고 디스플레이 출력 직전에 감마 보정을 적용해야 합니다.
