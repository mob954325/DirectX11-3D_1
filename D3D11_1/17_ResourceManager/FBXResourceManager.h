#pragma once
#include <map>
#include <string>
#include <memory>

#include "Mesh.h"
#include "TextureLoader.h"
#include "SkeletonInfo.h"
#include "Animation.h"

struct StaticMeshAsset
{
	SkeletonInfo skeletalInfo;
	std::vector<Animation> animations;
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	std::string directory = "";
};

class FBXResourceManager
{
	FBXResourceManager() = default;
	~FBXResourceManager() = default;

	FBXResourceManager(const FBXResourceManager&) = delete;
	FBXResourceManager& operator=(const FBXResourceManager&) = delete;

	// 해당 매니저에서 fbx를 읽는다.
	// 이미 읽은 fbx는 map에 저장된다.
	std::map<std::string, std::weak_ptr<StaticMeshAsset>> assets;

	// texture 불러오기 위한 device, deviceContext
	ComPtr<ID3D11Device> m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;

	void ProcessNode(std::shared_ptr<StaticMeshAsset>& pAsset, aiNode* pNode, const aiScene* pScene);
	Mesh ProcessMesh(std::shared_ptr<StaticMeshAsset>& pAsset, aiMesh* pMesh, const aiScene* pScene);
	void ProcessBoneWeight(std::shared_ptr<StaticMeshAsset>& pAsset, aiMesh* pMesh);
	std::vector<Texture> loadMaterialTextures(std::shared_ptr<StaticMeshAsset>& pAsset, aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
	void loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture);

public:
	static FBXResourceManager& Instance()
	{
		static FBXResourceManager manager;
		return manager;
	}

	std::shared_ptr<StaticMeshAsset> LoadFBXByPath(ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, const aiScene* pScene, std::string path);
};
