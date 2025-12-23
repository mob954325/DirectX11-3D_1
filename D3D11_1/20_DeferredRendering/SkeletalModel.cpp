#include "SkeletalModel.h"
#include "../Common/Helper.h"
#include "../Common/TimeSystem.h"

SkeletalModel::SkeletalModel()
{
}

SkeletalModel::~SkeletalModel()
{
}

bool SkeletalModel::Load(HWND hwnd, ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string filename)
{
	this->directory = filename.substr(0, filename.find_last_of("/\\"));
	this->m_pDevice = pDevice;
	this->m_pDeviceContext = pDeviceContext;
	this->hwnd = hwnd;

	// 리소스 매니저에서 FBX 정보 가져오기 -> 어디서?
	modelAsset = FBXResourceManager::Instance().LoadFBXByPath(pDevice, pDeviceContext, filename);

	// 인스턴스 데이터 생성하기 ( 본 데이터 )
	CreateBoneInfos();

	return true;
}

void SkeletalModel::Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext, ComPtr<ID3D11Buffer>& pMatBuffer)
{
	m_pDeviceContext->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, &m_BonePoses, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pBoneOffsetBuffer.Get(), 0, nullptr, &modelAsset->m_BoneOffsets, 0, 0);

	m_pDeviceContext->VSSetConstantBuffers(3, 1, m_pBonePoseBuffer.GetAddressOf());
	m_pDeviceContext->VSSetConstantBuffers(4, 1, m_pBoneOffsetBuffer.GetAddressOf());

	TransformBuffer tb = {};

	tb.isRigid = modelAsset->skeletalInfo.IsRigid();
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
	for (auto& bone : m_bones)
	{
		// 애니메이션 업데이트
		if (bone.m_boneAnimation.m_boneName != "")
		{
			Vector3 positionVec = Vector3::Zero;
			Vector3 scaleVec = Vector3::Zero;
			Quaternion rotationQuat = Quaternion::Identity;
			bone.m_boneAnimation.Evaluate(m_progressAnimationTime, positionVec, rotationQuat, scaleVec);

			if (positionVec != Vector3::Zero || rotationQuat != Quaternion::Identity || scaleVec != Vector3::Zero) // 움직이지 않는 본들은 갱신 안함
			{
				Matrix mat = Matrix::CreateScale(scaleVec) * Matrix::CreateFromQuaternion(rotationQuat) * Matrix::CreateTranslation(positionVec);
				bone.m_localTransform = mat.Transpose();
			}
		}

		// 위치 갱신
		if (bone.m_parentIndex != -1)
		{
			bone.m_worldTransform = m_bones[bone.m_parentIndex].m_worldTransform * bone.m_localTransform;
		}
		else
		{
			bone.m_worldTransform = bone.m_localTransform;
		}

		m_BonePoses.modelMatricies[bone.m_index] = bone.m_worldTransform;
	}	
}

void SkeletalModel::Close()
{
}

void SkeletalModel::GetBuffer(ComPtr<ID3D11Buffer>& pTransform, ComPtr<ID3D11Buffer>& pBonePose, ComPtr<ID3D11Buffer>& pBoneOffset)
{
	m_pTransformBuffer = pTransform;
	m_pBonePoseBuffer = pBonePose;
	m_pBoneOffsetBuffer = pBoneOffset;
}

void SkeletalModel::CreateBoneInfos()
{
	int size = modelAsset->skeletalInfo.m_bones.size();
	for (int i = 0; i < size; i++)
	{
		string boneName = modelAsset->skeletalInfo.m_bones[i].name;
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
		bool hasAnim = !modelAsset->animations.empty();
		if (parentBoneIndex != -1 && hasAnim)
		{
			modelAsset->animations[m_animationIndex].GetBoneAnimationByName(boneName, boneAnim);
			bone.m_boneAnimation = boneAnim;	// 임시 -> 0번째 애니메이션 받기
		}

		m_bones.push_back(bone); // -> 할당 겁나됨
	}
}