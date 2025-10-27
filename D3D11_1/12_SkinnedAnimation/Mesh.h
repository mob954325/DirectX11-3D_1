#pragma once
#include <wrl/client.h> // comptr
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <d3d11_1.h>
#include <directxtk/SimpleMath.h> // directXmath 대신 사용
#include "../Common/Helper.h"

using namespace std;
using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

const string TEXTURE_DIFFUSE = "texture_diffuse";
const string TEXTURE_EMISSIVE = "texture_emissive";
const string TEXTURE_NORMAL = "texture_normal";
const string TEXTURE_SPECULAR = "texture_specular";

struct BoneWeightVertex
{
    Vector3 position;
    Vector2 texture;
    Vector3 tangent;
    Vector3 bitangent;
    Vector3 normal;
	int BlendIndeces[4] = {};	// 참조하는 본 인덱스들
	float BlendWeight[4] = {};	// 가중치의 총 합은 1이여야한다.
};

struct Texture
{
	string type;
	string path;

	ComPtr<ID3D11ShaderResourceView> pTexture = nullptr;
};

struct Material
{
    Vector4 ambient;
    Vector4 diffuse;
    Vector4 specular;

    BOOL hasDiffuse = false;
    BOOL hasEmissive = false;
    BOOL hasNormal = false;
    BOOL hasSpecular = false;
};

class Mesh
{
public:
	vector<BoneWeightVertex> vertices;
	vector<UINT> indices;
	vector<Texture> textures;

	ComPtr<ID3D11Device> m_pDevice;
	int refBoneIndex = -1;

	Mesh(ComPtr<ID3D11Device>& dev, const std::vector<BoneWeightVertex>& vertices, const std::vector<UINT>& indices, const std::vector<Texture>& textures) :
		vertices(vertices),
		indices(indices),
		textures(textures),
		m_pDevice(dev),
		m_pVertexBuffer(nullptr),
		m_pIndexBuffer(nullptr)
	{
		this->setupMesh(this->m_pDevice);
	}

    void Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext);

	Material& GetMaterial();

private:
	Material material;

	ComPtr<ID3D11Buffer> m_pVertexBuffer{};
	ComPtr<ID3D11Buffer> m_pIndexBuffer{};

    // Functions
    // Initializes all the buffer objects/arrays
    void setupMesh(ComPtr<ID3D11Device>& dev);

	void ProcessTextureByType(ComPtr<ID3D11DeviceContext>& pDeviceContext, int index);
};