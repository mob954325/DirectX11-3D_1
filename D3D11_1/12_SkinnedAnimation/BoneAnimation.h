#pragma once
#include <vector>
#include <algorithm>
#include <assimp\scene.h>
#include "AnimationKey.h"


using namespace std;

class BoneAnimation
{
public:
	string m_boneName;				// 사용하는 본 이름
	vector<AnimationKey> m_keys;	// 채널(mChaneels)에 저장되어 있는 키 값들
	// string interpolationType		

	void CreateKeys(aiNodeAnim* pAiNodeAnim, float perTick)
	{
		// 일단 키 값들 개수와 각 키에 대한 시간이 모두 동일하다고 가정하고 작성
		int keyNum = pAiNodeAnim->mNumPositionKeys;
		for (int i = 0; i < keyNum; i++)
		{
			AnimationKey key;
			key.m_time = pAiNodeAnim->mPositionKeys[i].mTime / perTick;
			key.m_position =
			{
				pAiNodeAnim->mPositionKeys[i].mValue.x,
				pAiNodeAnim->mPositionKeys[i].mValue.y,
				pAiNodeAnim->mPositionKeys[i].mValue.z
			};

			key.m_rotation =
			{
				pAiNodeAnim->mRotationKeys[i].mValue.x,
				pAiNodeAnim->mRotationKeys[i].mValue.y,
				pAiNodeAnim->mRotationKeys[i].mValue.z,
				pAiNodeAnim->mRotationKeys[i].mValue.w
			};

			key.m_scale =
			{
				pAiNodeAnim->mScalingKeys[i].mValue.x,
				pAiNodeAnim->mScalingKeys[i].mValue.y,
				pAiNodeAnim->mScalingKeys[i].mValue.z
			};

			m_keys.push_back(key);
		}
	}

	template <typename T>
	T Clamp(T value, T min, T max)
	{
		return (value < min) ? min : (value > max) ? max : value;
	}

	void Evaluate(float time, Vector3& outPosition, Quaternion& outRotation, Vector3& outScale)
	{
		if (m_keys.size() < 2) // check has anim
			return;

		// 키프레임 쌍 탐색
		size_t index = 1;
		while (index + 1 < m_keys.size() && time >= m_keys[index + 1].m_time) // 보간 종료 지점 찾기 ( index + 1 )
		{
			index++;
		}

		// 인덱스 범위 확인
		if (index + 1 >= m_keys.size())
		{
			index = m_keys.size() - 2;
		}

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
	};
};