#include "Animation.h"
#include <stdexcept>

void Animation::CreateBoneAnimation(aiAnimation* pAiAnimation)
{
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

}

bool Animation::GetBoneAnimationByName(string boneName, BoneAnimation& out)
{
	auto anim = m_mappingBoneAnimations.find(boneName);
	if (anim == m_mappingBoneAnimations.end())
	{
		return false;
	}

	int index = anim->second;
	out = m_boneAnimations[index];

	return true;
}
