#include "Bone.h"

void Bone::CreateBone(string objName, int parentIndex, int boneIndex, Matrix worldMat, Matrix localMat)
{
	name = objName;
	m_parentIndex = parentIndex;
	m_index = boneIndex;

	m_worldTransform = worldMat;
	m_localTransform = localMat;
}