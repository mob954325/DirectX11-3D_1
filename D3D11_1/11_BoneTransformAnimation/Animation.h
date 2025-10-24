#pragma once
#include <string>
#include <vector>
#include "BoneAnimation.h"
#include <map>

using namespace std;

class Animation
{
public:
	map<string, int> m_mappingBoneAnimations; 
	vector<BoneAnimation> m_boneAnimations;		// 키 프레임에 등록된 본 위치와 시간 정보들
	float m_duration;							// 최종 시간
	string m_name;								// 애니메이션 이름

	void CreateBoneAnimation(aiAnimation* pAiAnimation);
	bool GetBoneAnimationByName(string boneName, BoneAnimation& out);
};

