#include "Bone.h"

void Bone::CreateBone(string objName, int parentIndex, int boneIndex, Matrix localMat, Matrix modelMat)
{
	name = objName;
	m_parentIndex = parentIndex;
	m_index = boneIndex;

	m_localTransform = localMat;
	m_ModelTransform = modelMat;
}