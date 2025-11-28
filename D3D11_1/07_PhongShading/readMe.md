## 1. 프로젝트 개요
셰이딩 모델 중 하나인 PhongShading 모델을 구현을 합니다.

Phong Shading에서 환경광 ( ambient ), 주변광 ( diffuse ), 정반사광 ( specular ) 이 어떻게 적용되는지 이해합니다.

PhongShading과 Blinn-Phong Shading 연산의 차이를 비교해봅니다.

## 2. 핵심 기술 포인트

* 환경광 (Ambient) 결정
  - 재질 환경광 × 빛 환경광
  - 음영이 있는 면에도 완전히 검게 보이지 않도록 기본 밝기 제공
  - 전역 조명(Global Illumination)의 간단한 근사치  
  
* 확산광 (Diffuse) 결정
  - 공식: max(0, dot(-lightDir, normal)) × 재질 확산광 × 빛 확산광 × 빛 색상
  - 텍스처 색상도 곱하여 최종 확산광 결정
  - Lambert's Cosine Law 기반  
  
* 정반사광 (Specular) 결정
  - **Phong 방식:**
    공식: pow(max(0, dot(reflectVec, viewVec)), shininess) × 재질 정반사광 × 빛 정반사광
    - reflect() 함수로 빛의 반사 벡터 계산
    - 반사 벡터와 시선 벡터의 각도로 하이라이트 강도 결정
    
  - **Blinn-Phong 방식:**
    공식: pow(max(0, dot(halfVec, normal)), shininess) × 재질 정반사광 × 빛 정반사광
    - Half Vector = normalize(viewVec + (-lightDir))
    - reflect() 연산 불필요 → 더 빠름
    - 시각적으로 Phong보다 약간 더 넓은 하이라이트  

* 재질(Material) 시스템 구현
  - 별도의 Constant Buffer (register(b1))로 재질 속성 관리
  - 오브젝트마다 다른 재질 적용 가능
  - Ambient, Diffuse, Specular 색상 독립적으로 조정  

* 카메라 위치 전달  
  - Specular 계산에 필요한 View Vector를 위해
  - Constant Buffer에 CameraPos 추가  

## 3. 그래픽스 파이프라인에서의 위치

- Input Assembler:
  Vertex 구조체: position, normal, texture (UV)
  재질 정보를 별도 Constant Buffer(b1)로 관리

- Vertex Shader:
  1. 정점을 World Space로 변환 → PS에 전달 (Specular용)
  2. 법선을 World Space로 변환 → PS에 전달
  3. MVP 변환으로 최종 위치 계산
  4. UV 좌표를 그대로 PS로 전달
  
  ※ 주의: 비균등 스케일 시 법선 변환에 역전치 행렬 필요  
         (이 프로젝트는 균등 스케일만 사용)

- Pixel Shader (Phong 또는 Blinn-Phong):  
  
  [공통 단계]  
  1. 텍스처 샘플링: txDiffuse.Sample(sampler, UV)
  2. 법선 정규화: normalize(input.normal)
  3. 환경광 계산: matAmbient × lightAmbient
  4. Diffuse Factor 계산: max(0, dot(-lightDir, normal))
  
  [Diffuse Factor > 0일 때만 계산]  
  5a. 확산광 계산:  
      texture × diffuseFactor × matDiffuse × lightDiffuse × lightColor
  
  5b. 정반사광 계산 (방식이 다름):  
      
      **Phong:**
      - reflectVec = reflect(lightDir, normal)
      - specFactor = pow(max(0, dot(reflectVec, viewVec)), shininess)
      
      **Blinn-Phong:**
      - halfVec = normalize(viewVec + (-lightDir))
      - specFactor = pow(max(0, dot(halfVec, normal)), shininess)
      
      finalSpecular = specFactor × matSpecular × lightSpecular × lightColor
  
  6. 최종 색상: ambient + diffuse + specular
     알파값은 텍스처 알파 사용
    

## 4. 구현에서 중요한 지점

1. 빛과 물체의 각 환경광, 확산광, 정반사광 값 GPU에 넘겨주기
    
    빛 내용 추가
    
    ```cpp
    cbuffer ConstantBuffer : register(b0)
    {
        matrix World; // 큐브 월드 
        matrix View;  // 카메라 뷰 
        matrix Projection; // 카메라 투영
        
        float4 LightDirection; // 빛의 방향 ( 정규화됨 )
        float4 LightColor;    // 빛의 색갈
            
        float4 LightAmbient; // 환경광
        float4 LightDiffuse; // 난반사
        float4 LightSpecular; // 정반사
        
        float Shininess; // 광택지수
        float3 CameraPos; // 카메라 위치
    }
    ```
    
    오브젝트가 사용할 머터리얼 정보
    
    ```cpp
    cbuffer Material : register(b1)
    {
        float4 matAmbient; // 큐브의 주변광
        float4 matDiffuse; // 큐브의 확산광
        float4 matSpecular;  // 큐브의 정반사광
    };
    ```
    

2. 환경광 ( Ambient ) 계산하기
    
    ```cpp
    float4 finalAmbient = matAmbient * LightAmbient;
    ```
    

3. 확산광 ( Diffuse ) 계산하기
    
    ```cpp
    float4 finalTexture = txDiffuse.Sample(samLinear, input.Tex);  // 텍스처 샘플링
    
    float4 finalDiffuse = finalTexture * diffuseFactor * matDiffuse * LightDiffuse * LightColor;
    ```
    

4. 정반사광 ( Sepcular ) 계산하기
    1. Phong 
    
    ```cpp
    // Phong
    float3 reflectionVector = reflect(normalize((float3)LightDirection), norm); // 
            
    // reflection calculates 
    // float3 i = normalize((float3) LightDirection);
    // float3 n = norm; 
    // float3 reflectionVector = i - 2 * n * dot(i, n);
            
    float specFactor = pow(max(dot(reflectionVector, normalize(CameraPos - input.World)), 0.0f), Shininess);
            
    finalSpecular = specFactor * matSpecular * LightSpecular * LightColor;
    ```
    ![phongimg](../../document/Resource/Projects/PhongShading/Phong1.png)  


    b. Blinn-Phong
    
    ```cpp
    // 정반사광        
    float3 viewVector = CameraPos - input.World;        
    float3 halfVector = viewVector + -(float3)LightDirection;        
    float specularFactor = pow(saturate(dot(norm, normalize(halfVector))), Shininess);
    
    finalSpecular = specularFactor * matSpecular * LightSpecular * LightColor;
    ```
    ![phongimg](../../document/Resource/Projects/PhongShading/Blinn-phong1.png)  

5. 출력할 색상 반환하기
    
    ```cpp
    float4 finalColor = finalAmbient + finalDiffuse + finalSpecular; // 최종 색
    finalColor.a = finalTexture.a;
        
    return finalColor;
    ```
    

## 5. 개발 과정에서 겪은 문제 & 해결 

1. 정반사광이 그림자에서 발생함
    - 문제 :
        
        정반사 광이 빛이 없는 곳인 그림자 면에서 발생
        
    - 원인 :
        
        정반사광은 빛의 반사와 상관없이 half벡터나 시선벡터의 값으로 연산하기 때문에 생겼었다.
        
    - 해결 :
        
        DiffuseFactor는 dot연산을 수행한 빛의 반사율인데 이 값이 양수 일 때만 계산하도록 분기 추가
        

## 6. 실행 결과


https://github.com/user-attachments/assets/67c62c0a-69d6-431f-a6b6-1ef3fe2a1c05


## 7. 배운 점

- Phong Shading Model의 빛 출력 방식
    - 환경광 - 간접광을 포함하는 전역 조명 ( Global Illumination ) 을 간단하게 표현한 방식
    - 확산광 - 직접광이 표면에서 특정 방향으로 집중되지 않고, 모든 방향에서 균일하게 퍼지는 빛
    - 정반사광 - 직접광이 표면에서 입사/반사각이 같은 반사하는 빛
    
- Phong과 Blinn-Phong의 연산 차이
    - Blinn-Phong은 빛 반사 연산을 수행하지 않고 dot 연산을 수행하기 때문에 Phong 보다 빠르다.
