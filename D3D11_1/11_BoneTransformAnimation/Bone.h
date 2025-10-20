#pragma once
#include <string>
#include <directxtk/SimpleMath.h>

using namespace std;
using namespace DirectX::SimpleMath;

class Bone
{
public:
	void CreateBone(string objName, int parentIndex, int boneIndex, Matrix localMat, Matrix modelMat);


// private: -> 편의를 위해 public 설정
	Matrix m_localTransform;
	Matrix m_ModelTransform;

	string name = "";		// 해당 bone의 이름

	int m_parentIndex = -1; // 계층 구조에서의 부모 인덱스
	int m_index = -1;		// 
};