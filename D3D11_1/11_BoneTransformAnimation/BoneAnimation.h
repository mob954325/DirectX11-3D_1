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

	void CreateKeys(aiNodeAnim* pAiNodeAnim)
	{
		// 일단 키 값들 개수와 각 키에 대한 시간이 모두 동일하다고 가정하고 작성
		int keyNum = pAiNodeAnim->mNumPositionKeys;
		for (int i = 0; i < keyNum; i++)
		{
			AnimationKey key;
			key.m_time = pAiNodeAnim->mPositionKeys[i].mTime;
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

	void Evaluate(float time, Vector3& position, Quaternion& rotation, Vector3& scale)
	{
		if (m_keys.size() < 2)
			return;

		// 키프레임 쌍 탐색
		size_t index = 0;
		while (index + 1 < m_keys.size() && time >= m_keys[index + 1].m_time)
		{
			index++;
		}

		if (index + 1 >= m_keys.size())
		{
			index = m_keys.size() - 2; // 마지막 두 키프레임으로 보간
		}

		const auto& keyA = m_keys[index];
		const auto& keyB = m_keys[index + 1];

		float duration = keyB.m_time - keyA.m_time;
		if (duration <= 0.0f) return;

		float t = (time - keyA.m_time) / duration;
		t = Clamp(t, 0.0f, 1.0f);

		position = Vector3::Lerp(keyA.m_position, keyB.m_position, t);
		rotation = Quaternion::Slerp(keyA.m_rotation, keyB.m_rotation, t);
		scale = Vector3::Lerp(keyA.m_scale, keyB.m_scale, t);
	};
};