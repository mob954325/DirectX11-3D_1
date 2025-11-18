#pragma once
#include <map>
#include <string>
#include <memory>

#include "Mesh.h"
#include "TextureLoader.h"
#include "SkeletonInfo.h"
#include "Animation.h"

struct BoneOffsetBuffer
{
	Matrix boneOffset[256];
};

struct FBXResourceAsset
{
	SkeletonInfo skeletalInfo;
	std::vector<Animation> animations;
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	std::string directory = "";
	BoneOffsetBuffer m_BoneOffsets{}; // skeletalInfo 본 위치 정보
};

class FBXResourceManager
{
	FBXResourceManager() = default;
	~FBXResourceManager() = default;

	FBXResourceManager(const FBXResourceManager&) = delete;
	FBXResourceManager& operator=(const FBXResourceManager&) = delete;

	// 해당 매니저에서 fbx를 읽는다.
	// 이미 읽은 fbx는 map에 저장된다.
	std::map<std::string, std::weak_ptr<FBXResourceAsset>> assets;

	// texture 불러오기 위한 device, deviceContext
	ComPtr<ID3D11Device> m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;

	// 에셋 내용 로드 함수들
	void ProcessNode(std::shared_ptr<FBXResourceAsset>& pAsset, aiNode* pNode, const aiScene* pScene);
	Mesh ProcessMesh(std::shared_ptr<FBXResourceAsset>& pAsset, aiMesh* pMesh, const aiScene* pScene);
	void ProcessBoneWeight(std::shared_ptr<FBXResourceAsset>& pAsset, aiMesh* pMesh);
	std::vector<Texture> loadMaterialTextures(std::shared_ptr<FBXResourceAsset>& pAsset, aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
	void loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture);

public:
	static FBXResourceManager& Instance()
	{
		static FBXResourceManager manager;
		return manager;
	}

	std::shared_ptr<FBXResourceAsset> LoadFBXByPath(ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string path);
};
