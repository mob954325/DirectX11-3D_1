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

class SkeletalModel
{
public:
	SkeletalModel();
	~SkeletalModel();

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

	std::string directory{};				// 로드한 파일이 위차한 폴더명
	std::vector<Mesh> meshes{};				// 로드한 매쉬
	std::vector<Texture> texturesLoaded{};	// 로드된 텍스처 모음
	std::vector<Bone> bones{};				// 로드된 모델의 본 모음 -> 계층 구조에 있는 오브젝트들
	std::map<string, int> bonesByIndex{};	// 본 이름으로 인덱스로 가져오기 위한 map

	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
	void loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture);
};

