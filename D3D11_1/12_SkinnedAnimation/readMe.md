# 프로젝트명

## 1. 프로젝트 개요
리깅 ( Rigging )된 Skinned Skeletal Mesh 애니메이션을 구현합니다.

정점마다 최대 4개 본의 영향을 받으며(boneIndices, boneWeights),
관절 부위(팔꿈치, 무릎 등)가 자연스럽게 구부러집니다.

## 2. 핵심 기술 포인트

- 정점 구조체에 Bone 가중치 정보 추가
    - boneIndices[4]: 영향을 주는 본의 인덱스 4개
    - boneWeights[4]: 각 본의 영향력 (총합 = 1.0)
    - aiProcess_LimitBoneWeights로 최대 4개로 제한
- 가중치에 영향 받는 위치 값을 사용합니다.
    
    → BoneOffset과 BonePose 값을 저장해 연관된 요소 값을 누적 합하여 월드 위치로 변환합니다.
    
- Skinning 계산 (Vertex Shader)
    - 각 본마다: OffsetPose = BoneOffset × BonePose
    - 4개 본을 가중치로 누적
- aiProcess_LimitBoneWeights 플래그
    - 정점당 본 영향 최대 4개로 제한
    - GPU 효율성 (일부 정점은 10개 이상 영향받을 수 있음)
    - 영향력 작은 본은 제거 후 가중치 재정규화

## 3. 그래픽스 파이프라인에서의 위치
- Input Assembler

- 정점 구조체 확장

```cpp
struct BoneWeightVertex
{
    Vector3 position;
    Vector2 texture;
    Vector3 tangent;
    Vector3 bitangent;
    Vector3 normal;
    
	int BlendIndeces[4] = {};	// 참조하는 본 인덱스들
	float BlendWeights[4] = {};	// 가중치 (총합 = 1.0)
}
```

- 본의 local 위치 값과 애니메이션 업데이트 위치 값 저장 버퍼를 추가.

```cpp
cbuffer BonePoseMatrix : register(b3) 
{
    matrix bonePose[256]; // skeleton pose matrix -> animation updated matrix
}

cbuffer BoneOffsetMatrix : register(b4)
{
    matrix boneOffset[256]; // model transform
}
```

- Vertex Shader

```cpp
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
//....
```

→ Vertex Local → Bone Space (Offset)
→ Bone Space → World Space (Pose)
→ 4개 본 결과를 가중치로 합성
→ Model World 변환 

## 4. 구현에서 중요한 지점

- 정점 구조체 내용 추가

```cpp
struct BoneWeightVertex
{
    // ...
		int BlendIndeces[4] = {};	// 참조하는 본 인덱스들
		float BlendWeights[4] = {};	// 가중치의 총 합은 1이여야한다.	
}
```

- 왜 4개로 제한?
→ GPU 레지스터 효율 (4개 = 128비트)
→ 실시간 성능과 품질의 균형점
→ 대부분 정점은 2~3개 본으로 충분

- FBX에서 Bone Weight 추출

```cpp
   void ProcessBoneWeight(aiMesh* pMesh) {
       for (UINT i = 0; i < pMesh->mNumBones; i++) {
           aiBone* pAiBone = pMesh->mBones[i];
           UINT boneIndex = m_pSkeletonInfo->GetBoneIndexByName(pAiBone->mName.C_Str());

           // 이 본이 영향주는 정점들 순회
           for (UINT j = 0; j < pAiBone->mNumWeights; j++) {
               UINT vertexId = pAiBone->mWeights[j].mVertexId;
               float weight = pAiBone->mWeights[j].mWeight;

               // 정점에 본 데이터 추가
               m_meshes.back().vertices[vertexId].AddBoneData(boneIndex, weight);
           }
       }
   }

```

- aiMesh->mBones: 이 메쉬에 영향주는 본 배열
- aiBone->mWeights: 이 본이 영향주는 정점과 가중치

- BoneOffset과 Pose 행렬 gpu로 넘겨주기

```cpp
	m_pDeviceContext->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, &m_BonePoses, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pBoneOffsetBuffer.Get(), 0, nullptr, &m_BoneOffsets, 0, 0);

	m_pDeviceContext->VSSetConstantBuffers(3, 1, m_pBonePoseBuffer.GetAddressOf());
	m_pDeviceContext->VSSetConstantBuffers(4, 1, m_pBoneOffsetBuffer.GetAddressOf());
```

→ Pose: 매 프레임 변경 (애니메이션)
→ Offset: 로드 시 한 번만 설정 (불변)

- aiProcess_LimitBoneWeights 플래그

```cpp
   unsigned int importFlag =
       aiProcess_Triangulate |
       aiProcess_LimitBoneWeights |  // 본 영향 4개로 제한
       aiProcess_ConvertToLeftHanded;
```

- 왜 필요?
→ 일부 정점은 10개 이상 본의 영향 받을 수 있음
→ GPU는 4개가 최적 (레지스터 효율)
→ Assimp가 자동으로 가장 영향력 큰 4개 선택 후 재정규화

## 5. 개발 과정에서 겪은 문제 & 해결

- 애니메이션 실행 시 캐릭터 포즈가 찢어지고 깨짐
    - 문제:
    팔다리가 이상한 방향으로 늘어나거나 꼬임
    관절 부위가 완전히 분리되어 보임
    일부 정점은 원점(0,0,0)으로 수축
    - 원인:
    Bone Offset Matrix를 적용하지 않음
    
    ```
      // 잘못된 방식
      worldPos = mul(localPos, bonePose[boneIndex]);
    
    ```
    
    정점의 Local Space가 본의 공간과 다름
    → 팔꿈치 정점은 "팔꿈치 본 기준 (0.5, 0, 0)"에 있어야 하는데
    → 그냥 월드 좌표로 변환하면 엉뚱한 곳에 배치
    
    - 해결:
    Offset Matrix로 먼저 Bone Space로 변환 후 Pose 적용
    
    ```
      // 올바른 방식
      OffsetPose = mul(boneOffset[i], bonePose[i]);
      worldPos = mul(localPos, OffsetPose);
    
    ```
    

## 6. 실행 결과

## 7. 배운 점

- Skinned Animation의 수학적 원리
    - 각 정점은 최대 4개 본의 영향을 받음
    - 최종 위치 = Σ(가중치[i] × 본변환[i] × 정점위치)
    - 가중치 총합 = 1.0 (정규화 필수)
- Bone Offset과 Bone Pose의 역할
    - **Bone Offset (불변):**
    → Vertex Local Space → Bone Space 변환
    → Inverse(Bind Pose World Transform)
    → 3D 모델링 툴에서 리깅 시 결정
    → FBX aiBone->mOffsetMatrix로 저장
    - **Bone Pose (매 프레임 변경):**
    → Bone Space → World Space 변환
    → 애니메이션 키프레임 보간 결과
    → 이전 프로젝트의 World Transform과 동일