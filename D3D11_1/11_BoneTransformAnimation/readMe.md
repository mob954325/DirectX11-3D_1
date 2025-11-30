## 1. 프로젝트 개요

이번에는 Skeletal Animation(스켈레탈 애니메이션)을 구현하여
캐릭터가 움직이는 애니메이션을 표현합니다.

FBX 파일에서 본(Bone) 계층 구조와 애니메이션 데이터를 추출하고,
키프레임 간 보간(Interpolation)을 통해 부드러운 애니메이션을 생성합니다.

이 프로젝트에서는 Skeletal Animation 중 Rigid Body 방식을 구현하며,
각 메쉬 전체가 하나의 본에 종속되어 움직입니다.

## 2. 핵심 기술 포인트

- FBX에서 animation 데이터를 추출합니다.
    - aiAnimation의 이름, 틱, 시간, 키에 대한 정보를 저장합니다.
- 키프레임간 보간을 하여 애니메이션을 구성합니다
    - 키 데이터를 순회하면서 시간에 맞는 구간을 찾은 뒤 현재 인덱스와 현재 인덱스 + 1 사이의 값을 보간하여 위치, 회전, 스케일은 반환합니다.

## 3. 그래픽스 파이프라인에서의 위치
- Input Assembler

- 본 매트릭스 저장용 버퍼 추가

```cpp
cbuffer ModelMatrix : register(b3) // Skinning
{
    matrix modelMatricies[128]; // model space matricies
}
```

→ 해당 버퍼에는 각 인덱스마다 본의 매트릭스가 저장되어 있으며 Cpu업데이트를 반영함

- Vertex Shader

- 본 위치를 월드 위치로 변환 후 해당 값을 월드 값으로 사용한다.

```cpp
    Matrix finalWorld = mul(modelMatricies[refBoneIndex], World);
    output.Pos = mul(input.Pos, finalWorld);
    output.World = output.Pos;    
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
```

## 4. 구현에서 중요한 지점

- modelLoader에서 aiScene* 객체에서 aiAnimation 추출

```cpp
	m_name = pAiAnimation->mName.C_Str();
	m_tick = pAiAnimation->mTicksPerSecond;
	m_duration = pAiAnimation->mDuration / m_tick;

	// 본에 대한 키 애니메이션 저장
	for (int i = 0; i < pAiAnimation->mNumChannels; i++)
	{
		aiNodeAnim* pAiNodeAnim = pAiAnimation->mChannels[i];

		BoneAnimation boneAnim;
		boneAnim.m_boneName = pAiNodeAnim->mNodeName.C_Str();
		boneAnim.CreateKeys(pAiNodeAnim, m_tick);
		m_boneAnimations.push_back(boneAnim);
		m_mappingBoneAnimations.insert({ boneAnim.m_boneName, i });
	}
```

- aiAnimation: 애니메이션 전체 정보 (이름, 길이, 틱)
    - mChannels: 각 본의 키프레임 데이터 배열
    - 왜 Tick → 초 변환?
    → FBX는 틱 단위 저장 (예: 24틱/초, 30틱/초)
    → 게임은 실시간(초) 단위로 동작
    → Duration(틱) ÷ TicksPerSecond = Duration(초)

- animation class내에 evalate 함수
    - 현재 플레이시간 기준 초와 다음 1초간 사이를 보간함수를 이용해 애니메이션 보간
        
        ```cpp
        const auto& keyA = m_keys[index];			// 보간 시작 지점
        const auto& keyB = m_keys[index + 1];		// 보간 종료 지점
        
        float duration = keyB.m_time - keyA.m_time;	// 유효한 지점인지 검사
        if (duration <= 0.0f) return;
        
        float t = (time - keyA.m_time) / duration;
        t = Clamp(t, 0.0f, 1.0f);
        
        // calcuate out datas
        outPosition = Vector3::Lerp(keyA.m_position, keyB.m_position, t);
        outRotation = Quaternion::Slerp(keyA.m_rotation, keyB.m_rotation, t);
        outScale = Vector3::Lerp(keyA.m_scale, keyB.m_scale, t);
        ```
        
    - 왜 Lerp와 Slerp 다르게?
    → Position/Scale: 직선 보간 (Lerp)
    → Rotation: 구면 보간 (Slerp) - 짐벌락 방지  

    - t 계산 원리:
    → keyA = 1.0초, keyB = 2.0초, 현재 time = 1.3초
    → t = (1.3 - 1.0) / (2.0 - 1.0) = 0.3
    → 30% 지점의 값 반환

- 본 계층 구조 업데이트
    
    ```cpp
    void SkeletalModel::Update()
    {
    	if (!m_animations.empty())
    	{
    		// 1. 애니메이션 시간 업데이트
    		m_progressAnimationTime += GameTimer::m_Instance->DeltaTime();
    		m_progressAnimationTime = fmod(m_progressAnimationTime, m_animations[m_animationIndex].m_duration);	
    	}
    
    	ModelMatrixBuffer mmb{};
    	
    	// 2. 모든 본의 변환 계산
    	for (auto& bone : bones)
    	{
    		if (bone.m_boneAnimation.m_boneName != "")
    		{
    			Vector3 positionVec, scaleVec;
    			Quaternion rotationQuat;
    			bone.m_boneAnimation.Evaluate(m_progressAnimationTime, positionVec, rotationQuat, scaleVec);
    
    			Matrix mat = Matrix::CreateScale(scaleVec) * Matrix::CreateFromQuaternion(rotationQuat) * Matrix::CreateTranslation(positionVec);
    			bone.m_localTransform = mat.Transpose();
    		}
    
    		// 부모 본의 변환 적용
    		if (bone.m_parentIndex != -1)
    		{
    			bone.m_worldTransform = bones[bone.m_parentIndex].m_worldTransform * bone.m_localTransform;
    		}
    		else
    		{
    			bone.m_worldTransform = bone.m_localTransform;
    		}
    
    		mmb.modelMatricies[bone.m_index] = bone.m_worldTransform;
    	}	
    	
    	// 3. GPU로 전송
    	m_pDeviceContext->UpdateSubresource(m_pModelMetriciesBuffer.Get(), 0, nullptr, &mmb, 0, 0);
    	m_pDeviceContext->VSSetConstantBuffers(3, 1, m_pModelMetriciesBuffer.GetAddressOf());
    }
    ```
    
    - 부모 World × 자식 Local?
    → 부모가 회전하면 자식도 따라 회전해야 함
    → 예: 팔(부모)이 올라가면 손(자식)도 함께 올라감  
    
    - Transpose()?
    → DirectX Math는 Row-major
    → HLSL은 Column-major
    → 전치 행렬로 변환 필요  
    
    - CPU에서 계산?
    → 본 계층 구조는 순차적 계산 (부모→자식)
    → GPU는 병렬 처리에 특화, 순차 처리는 비효율적
    → CPU에서 계산 후 결과만 GPU 전송이 더 효율적

## 5. 개발 과정에서 겪은 문제 & 해결

- 애니메이션이 예상보다 느림
    
    - 문제: 애니메이션의 실행 시간이 예상 시간보다 긺.
    - 원인: 시간을 갱신해서 애니메이션을 실행하는데 저장된 duration 값은 Tick 기준 값이였음.
    - 해결: Duration / tick 으로 애니메이션 시간을 초 값으로 바꾼다.
    
- 캐릭터 애니메이션이 제대로 출력이 안됨
    
    - 문제: 모델이 한 곳에 뭉쳐서 출력된다.
    - 원인: 각 정점 위치 업데이트가 로컬 값만 업데이트 되었다.
    - 해결: 각 본의 위치를 갱신할 때마다 연관된 부모 위치를 기준으로 한 로컬 값을 계산해 Gpu에 넘겨준다.
    

## 6. 실행 결과

## 7. 배운 점

- Skeletal Animation 중 Rigid Body 방식
    - 각 메쉬 전체가 하나의 본에 종속
    - World Transform = Parent World × Local Transform
    - 계층 구조를 통한 연쇄 변환 (팔이 올라가면 손도 올라감)  

- 사원수(Quaternion) vs 오일러(Euler) 회전  
    - **오일러 각의 문제:**
    → 짐벌락(Gimbal Lock): 두 축이 겹쳐 한 축 자유도 상실
    → Lerp 보간 시 부자연스러운 회전 경로
    → 예: 350° → 10° 회전 시 340° 역회전  

    - **Quaternion의 장점:**
    → 4차원 단위 구면 위의 점으로 회전 표현
    → Slerp(구면 선형 보간)로 최단 경로 회전
    → 짐벌락 현상 완전히 방지
    → 모든 상용 게임 엔진이 Quaternion 사용  

- 키프레임 보간의 원리  
    - 키프레임: 특정 시간의 변환 값만 저장 (메모리 효율)
    - 보간: 키프레임 사이를 수학적으로 계산 (부드러운 애니메이션)
    - Lerp: 직선 보간 (Position, Scale)
    - Slerp: 구면 보간 (Rotation)  

- Tick vs 실시간(초) 변환  
    - FBX는 Tick 단위로 시간 저장 (예: 24틱/초, 30틱/초)
    - 게임은 Delta Time(초) 단위로 동작
    - Duration(틱) ÷ TicksPerSecond = Duration(초)  

- CPU vs GPU 역할 분담
    - CPU: 본 계층 계산, 키프레임 보간 (순차 처리)
    - GPU: 정점 변환 (병렬 처리)
    - Constant Buffer로 계산 결과만 전송