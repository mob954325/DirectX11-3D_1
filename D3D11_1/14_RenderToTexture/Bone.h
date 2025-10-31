#pragma once
#include <string>
#include <directxtk/SimpleMath.h>
#include "BoneAnimation.h"

using namespace std;
using namespace DirectX::SimpleMath;

class Bone
{
public:
	void CreateBone(string objName, int parentIndex, int boneIndex, Matrix worldMat, Matrix localMat);

	BoneAnimation m_boneAnimation;	// 

// private: -> 편의를 위해 public 설정
	Matrix m_worldTransform;
	Matrix m_localTransform;

	string name = "";		// 해당 bone의 이름

	int m_parentIndex = -1; // 계층 구조에서의 부모 인덱스
	int m_index = -1;		// 
};