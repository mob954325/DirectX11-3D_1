#include "SkeletonInfo.h"
#include <iostream>

BoneInfo SkeletonInfo::GetBoneInfoByIndex(int index)
{
    if (index < 0 || index >= m_bones.size())
        throw std::out_of_range("Bone index not Found" + index);

    return m_bones[index];
}

BoneInfo SkeletonInfo::GetBoneInfoByName(const string& boneName)
{
    auto mappedInfo = m_boneMappingTable.find(boneName);
    if (mappedInfo == m_boneMappingTable.end()) 
        throw std::out_of_range("Bone name not Found " + boneName);

    return m_bones[mappedInfo->second];
}

int SkeletonInfo::GetBoneIndexByName(const string& boneName)
{
	auto mappedInfo = m_boneMappingTable.find(boneName);
	if (mappedInfo == m_boneMappingTable.end())
		throw std::out_of_range("Bone name not Found " + boneName);

	return mappedInfo->second;
}

bool SkeletonInfo::CreateFromAiScene(const aiScene* pAiScene)
{
	if (pAiScene == nullptr) return false;

	aiNode* rootNode = pAiScene->mRootNode;

	CreateBoneInfoFromNode(rootNode);

    return true;
}

void SkeletonInfo::CreateBoneInfoFromNode(const aiNode* pAiNode)
{
	aiNode* parentNode = pAiNode->mParent;
	string parentBoneName = parentNode != nullptr ? parentNode->mName.C_Str() : "";
	string currentBoneName = pAiNode->mName.C_Str();

	BoneInfo parentBoneInfo{};
	int parentBoneIndex = -1;
	if (parentNode != nullptr)
	{
		parentBoneInfo = GetBoneInfoByName(parentBoneName);
	}

	Matrix localMat = Matrix(pAiNode->mTransformation.a1, pAiNode->mTransformation.a2, pAiNode->mTransformation.a3, pAiNode->mTransformation.a4,
		pAiNode->mTransformation.b1, pAiNode->mTransformation.b2, pAiNode->mTransformation.b3, pAiNode->mTransformation.b4,
		pAiNode->mTransformation.c1, pAiNode->mTransformation.c2, pAiNode->mTransformation.c3, pAiNode->mTransformation.c4,
		pAiNode->mTransformation.d1, pAiNode->mTransformation.d2, pAiNode->mTransformation.d3, pAiNode->mTransformation.d4);

	BoneInfo currInfo{};
	currInfo.name = currentBoneName;
	currInfo.parentBoneName = parentBoneName;
	currInfo.relativeTransform = localMat;

	m_bones.push_back(currInfo);
	m_boneMappingTable.insert({ currentBoneName, m_bones.size() - 1 });
	// m_meshMappingTable.insert({ currentBoneName, pAiNode->mesh}) // -> ???

	int currentBoneIndex = m_bones.size();
	int childCount = pAiNode->mNumChildren;
	for (int i = 0; i < childCount; i++)
	{
		CreateBoneInfoFromNode(pAiNode->mChildren[i]);
	}
}