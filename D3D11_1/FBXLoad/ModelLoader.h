#pragma once

#include <vector>
#include <d3d11_1.h>
#include <DirectXMath.h>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include "Mesh.h"
#include "TextureLoader.h"

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();

	Vector4 m_Ambient{ 1.0f, 1.0f, 1.0f, 1.0f }; // 환경광 반사 계수
	Vector4 m_Diffuse{ 1.0f, 1.0f, 1.0f, 1.0f }; // 난반사 계수
	Vector4 m_Specular{ 1.0f, 1.0f, 1.0f, 1.0f }; // 정반사 계수

	bool Load(HWND hwnd, ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string filename);
	void Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext, ComPtr<ID3D11Buffer>& pMatBuffer);

	void Close();
private:
	ComPtr<ID3D11Device> m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;
	std::vector<Mesh> meshes{};
	std::string directory{};
	std::vector<Texture> texturesLoaded{};
	HWND hwnd{};

	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
	void loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture);
};

