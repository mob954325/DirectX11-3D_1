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
#include "Bone.h"
#include "SkeletonInfo.h"

// skeletalInfo
// animations

// ____ Resource ____
// vector<SkeletalMeshSection
// vector<Material>
// vector<Animation>
// SkeletonInfo
//	|
//	ㄴ 본 정보?
// ___ instance data ___

class SkeletalModel
{
public:
	SkeletalModel();
	~SkeletalModel();

	Matrix m_world{};
	Vector3 m_Position{ 0.0f, 0.0f, 10.0f };
	Vector3 m_Rotation{};
	Vector3 m_Scale{ 1.0f, 1.0f, 1.0f };

	Vector4 m_Ambient{ 1.0f, 1.0f, 1.0f, 1.0f }; // 환경광 반사 계수
	Vector4 m_Diffuse{ 1.0f, 1.0f, 1.0f, 1.0f }; // 난반사 계수
	Vector4 m_Specular{ 1.0f, 1.0f, 1.0f, 1.0f }; // 정반사 계수

	bool Load(HWND hwnd, ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string filename);
	void Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext, ComPtr<ID3D11Buffer>& pMatBuffer);

	void Close();
private:
	ComPtr<ID3D11Device> m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;
	HWND hwnd{};

	ComPtr<ID3D11Buffer> m_pTransformBuffer{};
	ComPtr<ID3D11Buffer> m_pModelMetriciesBuffer{};

	std::string directory{};				// 로드한 파일이 위차한 폴더명
	std::vector<Mesh> meshes{};				// 로드한 매쉬
	std::vector<Texture> texturesLoaded{};	// 로드된 텍스처 모음
	std::vector<Bone> bones{};				// 로드된 모델의 본 모음 -> 계층 구조에 있는 오브젝트들

	SkeletonInfo m_skeletonInfo{};			// 모델에 사용할 본 정보 
	bool isRigid = false;

	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
	void loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture);
};

