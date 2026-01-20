## 1. 프로젝트 개요

3D 정점 데이터(큐브)를 생성하고 Model-View-Projection 변환을 적용하여 3D 공간상의 객체를 화면에 렌더링하는 기초 프로젝트입니다.

Depth Stencil View를 활용한 깊이 테스트와, 계층적 변환(부모-자식 관계)을 통해 행성 궤도 시스템을 구현했습니다.

## 2. 핵심 기술 포인트

- View, Projection 매트릭스를 생성하고 상수버퍼로 GPU에 전달
- Depth Stencil State 설정을 통한 깊이 테스트 활성화
- 계층적 변환 행렬 연산 (부모 월드 행렬 × 자식 로컬 변환)
- HLSL과 C++의 행렬 메모리 레이아웃 차이 대응 (전치 행렬)

## 3. 그래픽스 파이프라인에서의 위치

1. VS 단계
    - Model, View, Projection 연산을 수행하고 해당 결과를 PS에 넘겨줍니다.
2. OM 단계
    - Depth Stencil State 활성화로 깊이 테스트 수행합니다.

## 4. 구현에서 중요한 지점

1. Depth Stencil State 설정
    
    `D3D11_COMPARISON_LESS` 로 깊이 비교 함수를 설정해 Z값이 작은 ( 카메라에 가까운 ) 픽셀이 그려지도록 설정하기
    
    `CreateDepthStencilState()` 로 뎊스 스텐실 상태 만들기
    
    `OMSetDepthStencilState()` 로 OM 단계 뎊스 스텐실 상태 바인딩하기
    
2. 계층적 변환 구현
    - 자식 월드 행렬 = Scale × Rotate × Translate × 부모World
    - 왜?: 부모 좌표계 기준으로 자식이 변환되어 행성 궤도 효과 달성
    - 핵심: 행렬 곱셈 순서가 중요 (오른쪽→왼쪽 적용)
    
3. 행렬 전치 필요성
    - C++: 행 우선(Row-major), HLSL: 열 우선(Column-major)
    - XMMatrixTranspose() 호출로 메모리 레이아웃 차이 해결
    - 왜 필요?: mul(matrix, vector) 연산 시 올바른 결과를 위해

## 5. 개발 과정에서 겪은 문제 & 해결 

1. 행렬 곱셈 순서와 전치 행렬
    - 문제 : 
        큐브가 의도와 다르게 나타나지 않거나 찌그러짐 
        
    
    - 원인 :
        DirectX(C++)는 행 우선 HLSL은 열 우선으로 행렬을 해석

        mul(matrix, vector) 연산 시 메모리 레이아웃이 달라 결과가 다름
        
    - 해결 :
        상수 버퍼 업데이트 시 `1XMMatrixTranspose()`로 전치 후 전달
        
        변환 순서 : Scale → Rotate → Translate → ParentWorld 순으로 적용
        

## 6. 실행 결과
https://github.com/user-attachments/assets/308f7bc0-c200-48c0-8ed2-c7aa52d9f266


- 중앙 큐브: Y축 기준 자전
- 중간 큐브: 중앙 큐브 주위를 공전하며 자전
- 작은 큐브: 중간 큐브 주위를 공전
- ImGui로 각 큐브 위치, 카메라 위치/회전, FOV/Near/Far 실시간 조정 가능

## 7. 배운 점
- MVP 변환의 수학적 의미와 각 행렬의 역할 이해
- Depth Test가 3D 렌더링에서 필수적인 이유 체득
- 행렬 곱셈 순서가 변환 결과에 미치는 영향 파악
- C++과 HLSL의 행렬 메모리 레이아웃 차이 대응 방법
