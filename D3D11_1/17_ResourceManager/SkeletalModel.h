#pragma once

#include <vector>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <map>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include "Mesh.h"
#include "TextureLoader.h"
#include "SkeletonInfo.h"
#include "Animation.h"
#include "Bone.h"
#include "FBXResourceManager.h"

struct BonePoseBuffer
{
	Matrix modelMatricies[256];
};

struct BoneOffsetBuffer
{
	Matrix boneOffset[256];
};

class SkeletalModel
{
public:
	SkeletalModel();
	~SkeletalModel();

	Matrix m_world{};
	Vector3 m_Position{ 0.0f, 0.0f, 10.0f };
	Vector3 m_Rotation{};
	Vector3 m_Scale{ 1.0f, 1.0f, 1.0f };

	bool Load(HWND hwnd, ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string filename);
	void Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext, ComPtr<ID3D11Buffer>& pMatBuffer);
	void Update();

	void Close();

	// 애니메이션 관련 내용 - 디버그를 위해 public으로 옮김
	int m_animationIndex = 0;				// 실행 중인 애니메이션 인덱스
	float m_progressAnimationTime = 0.0f;		// 현재 애니메이션 시간 

	bool isAnimPlay = true;
	bool isRigid = true;

	// 리소스 데이터
	shared_ptr<StaticMeshAsset> modelAsset{};
private:
	ComPtr<ID3D11Device> m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;
	HWND hwnd{};

	// 인스턴스 데이터
	std::string directory{};				// 로드한 파일이 위차한 폴더명
	std::vector<Bone> m_bones{};				// 로드된 모델의 본 모음 -> 계층 구조에 있는 오브젝트들

	// 해당 모델의 상수 버퍼 내용
	BonePoseBuffer m_BonePoses{};
	BoneOffsetBuffer m_BoneOffsets{};

	// 버퍼들
	ComPtr<ID3D11Buffer> m_pTransformBuffer{};
	ComPtr<ID3D11Buffer> m_pBonePoseBuffer{};
	ComPtr<ID3D11Buffer> m_pBoneOffsetBuffer{};

	// 기능 함수
	void CreateBoneInfos(aiNode* pNode, const aiScene* pScene);
};

