#include "SkeletalModel.h"
#include "../Common/Helper.h"
#include "../Common/TimeSystem.h"

/// <summary>
/// 모델에서 사용할 트랜스폼 상수 버퍼 구조체
/// </summary>
struct TransformBuffer
{
	Matrix world;

	UINT isRigid;		// 1 : rigid, 0 : skinned
	UINT refBoneIndex;	// 리지드일 때 참조하는 본 인덱스
	FLOAT pad1;
	FLOAT pad2;
};

SkeletalModel::SkeletalModel()
{
}

SkeletalModel::~SkeletalModel()
{
}

bool SkeletalModel::Load(HWND hwnd, ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string filename)
{
	Assimp::Importer importer;

	unsigned int importFlag = aiProcess_Triangulate |	// 삼각형 변환
		aiProcess_GenNormals |				// 노말 생성
		aiProcess_GenUVCoords |				// UV 생성
		aiProcess_CalcTangentSpace |		// 탄젠트 생성
		aiProcess_LimitBoneWeights |		// 본의 영향을 받는 정점의 최대 개수 4개로 제한
		aiProcess_ConvertToLeftHanded;		// 왼손 좌표계로 변환

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
	
	const aiScene* pScene = importer.ReadFile(filename, importFlag);

	if (pScene == nullptr)
		return false;	

	// check rigid
	for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		if (pScene->mMeshes[i]->mNumBones > 0) // 본이 존재한다 -> skinned
		{
			isRigid = false;
			break;
		}
	}

	this->directory = filename.substr(0, filename.find_last_of("/\\"));
	this->m_pDevice = pDevice;
	this->m_pDeviceContext = pDeviceContext;
	this->hwnd = hwnd;

	// 트랜스폼 상수 버퍼 만들기
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(TransformBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pTransformBuffer.GetAddressOf()));

	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BonePoseBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pBonePoseBuffer.GetAddressOf()));

	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BoneOffsetBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pBoneOffsetBuffer.GetAddressOf()));

	// 리소스 매니저에서 FBX 정보 가져오기
	modelAsset = FBXResourceManager::Instance().LoadFBXByPath(pDevice, pDeviceContext, pScene, filename);

	// 인스턴스 데이터 생성하기 ( 본 데이터 )
	CreateBoneInfos(pScene->mRootNode, pScene);

	return true;
}

void SkeletalModel::Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext, ComPtr<ID3D11Buffer>& pMatBuffer)
{
	m_pDeviceContext->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, &m_BonePoses, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pBoneOffsetBuffer.Get(), 0, nullptr, &m_BoneOffsets, 0, 0);

	m_pDeviceContext->VSSetConstantBuffers(3, 1, m_pBonePoseBuffer.GetAddressOf());
	m_pDeviceContext->VSSetConstantBuffers(4, 1, m_pBoneOffsetBuffer.GetAddressOf());

	TransformBuffer tb = {};

	tb.isRigid = isRigid;
	m_world = m_world.CreateScale(m_Scale) *
			  m_world.CreateFromYawPitchRoll(m_Rotation) *
			  m_world.CreateTranslation(m_Position);
	tb.world = XMMatrixTranspose(m_world);

	int size = modelAsset->meshes.size();
	for (size_t i = 0; i < size; i++)
	{
		Material meshMaterial = modelAsset->meshes[i].GetMaterial();
		m_pDeviceContext->UpdateSubresource(pMatBuffer.Get(), 0, nullptr, &meshMaterial, 0, 0);		

		tb.refBoneIndex = modelAsset->meshes[i].refBoneIndex;
		
		m_pDeviceContext->UpdateSubresource(m_pTransformBuffer.Get(), 0, nullptr, &tb, 0, 0);
		m_pDeviceContext->VSSetConstantBuffers(2, 1, m_pTransformBuffer.GetAddressOf());
		m_pDeviceContext->PSSetConstantBuffers(1, 1, pMatBuffer.GetAddressOf());

		modelAsset->meshes[i].Draw(pDeviceContext);
	}
}

void SkeletalModel::Update()
{
	if (!modelAsset->animations.empty() && isAnimPlay)
	{
		m_progressAnimationTime += GameTimer::m_Instance->DeltaTime();
		m_progressAnimationTime = fmod(m_progressAnimationTime, modelAsset->animations[m_animationIndex].m_duration);
	}

	// pose 본 갱신
	for (int i = 0; i < m_bones.size(); i++)
	{
		// 애니메이션 업데이트
		if (m_bones[i].m_boneAnimation.m_boneName != "")
		{
			Vector3 positionVec, scaleVec;
			Quaternion rotationQuat;
			m_bones[i].m_boneAnimation.Evaluate(m_progressAnimationTime, positionVec, rotationQuat, scaleVec);

			Matrix mat = Matrix::CreateScale(scaleVec) * Matrix::CreateFromQuaternion(rotationQuat) * Matrix::CreateTranslation(positionVec);
			m_bones[i].m_localTransform = mat.Transpose();
		}

		// 위치 갱신
		if (m_bones[i].m_parentIndex != -1)
		{
			m_bones[i].m_worldTransform = m_bones[m_bones[i].m_parentIndex].m_worldTransform * m_bones[i].m_localTransform;
		}
		else
		{
			m_bones[i].m_worldTransform = m_bones[i].m_localTransform;
		}

		m_BonePoses.modelMatricies[m_bones[i].m_index] = m_bones[i].m_worldTransform;
	
		// update offsetMatrix
		Matrix offsetMat = Matrix::Identity;
		if (i > 0)
		{
			offsetMat = modelAsset->skeletalInfo.GetBoneOffsetByName(m_bones[i].name);
		}
		m_BoneOffsets.boneOffset[m_bones[i].m_index] = offsetMat;
	}		
}

void SkeletalModel::Close()
{
}

void SkeletalModel::CreateBoneInfos(aiNode* pNode, const aiScene* pScene)
{
	string boneName = pNode->mName.C_Str();
	BoneInfo boneInfo = modelAsset->skeletalInfo.GetBoneInfoByName(boneName);
	int boneIndex = modelAsset->skeletalInfo.GetBoneIndexByName(boneName);

	string parentBoneName = boneInfo.parentBoneName;
	BoneInfo parentBoneInfo;
	int parentBoneIndex = -1;
	if (parentBoneName != "")
	{
		parentBoneInfo = modelAsset->skeletalInfo.GetBoneInfoByName(parentBoneName);
		parentBoneIndex = modelAsset->skeletalInfo.GetBoneIndexByName(parentBoneName);
	}

	Matrix localMat = boneInfo.relativeTransform;
	Matrix worldMat = parentBoneIndex > 0 ? m_bones[parentBoneIndex].m_worldTransform * localMat : localMat;

	// Bone 정보 생성
	Bone bone;
	bone.CreateBone(boneName, parentBoneIndex, boneIndex, worldMat, localMat);	//...

	BoneAnimation boneAnim;
	bool hasAnim = pScene->mNumAnimations > 0;
	if (parentBoneIndex != -1 && hasAnim)
	{
		modelAsset->animations[m_animationIndex].GetBoneAnimationByName(boneName, boneAnim);
		bone.m_boneAnimation = boneAnim;	// 임시 -> 0번째 애니메이션 받기
	}

	m_bones.push_back(bone);

	for (UINT i = 0; i < pNode->mNumChildren; i++)
	{
		this->CreateBoneInfos(pNode->mChildren[i], pScene);
	}
}