#pragma once
#include <string>
#include <vector>
#include <map>

#include <assimp\scene.h>
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;
using namespace std;

/// <summary>
/// 본에 대한 관계 데이터
/// </summary>
class BoneInfo
{
public:
	string name{};				// 본 이름
	string parentBoneName{};	// 해당 본의 부모 이름 없으면 ""
	Matrix relativeTransform{};	// 해당 본 매트릭스 값
};

/// <summary>
/// boneInfo 매핑 데이터를 가지고 있는 클리스
/// </summary>
class SkeletonInfo
{
private:
	map<string, int> m_boneMappingTable;
	map<string, Matrix> m_bonesOffset;
	bool isRigid = true;

	void CreateBoneInfoFromNode(const aiNode* pAiNode);

public:
	vector<BoneInfo> m_bones;

	BoneInfo GetBoneInfoByIndex(int index);
	BoneInfo GetBoneInfoByName(const string& boneName);
	int GetBoneIndexByName(const string& boneName);
	Matrix GetBoneOffsetByName(const string& boneName);
	bool CreateFromAiScene(const aiScene* pAiScene);

	bool IsRigid() { return isRigid; };
};