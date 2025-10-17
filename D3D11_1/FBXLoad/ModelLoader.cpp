#include "ModelLoader.h"

ModelLoader::ModelLoader()
{
}

ModelLoader::~ModelLoader()
{
}

bool ModelLoader::Load(HWND hwnd, ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string filename)
{
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded);

	if (pScene == nullptr)
		return false;

	this->directory = filename.substr(0, filename.find_last_of("/\\"));

	this->m_pDevice = pDevice;
	this->m_pDeviceContext = pDeviceContext;
	this->hwnd = hwnd;

	processNode(pScene->mRootNode, pScene);

	return true;
}

void ModelLoader::Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext, ComPtr<ID3D11Buffer>& pMatBuffer)
{
	int size = meshes.size();
	for (size_t i = 0; i < size; i++)
	{
		Material meshMaterial = meshes[i].GetMaterial();
		meshMaterial.ambient = m_Ambient;
		meshMaterial.diffuse = m_Diffuse;
		meshMaterial.specular = m_Specular;

		m_pDeviceContext->UpdateSubresource(pMatBuffer.Get(), 0, nullptr, &meshMaterial, 0, 0);
		m_pDeviceContext->PSSetConstantBuffers(1, 1, pMatBuffer.GetAddressOf());

		meshes[i].Draw(pDeviceContext);
	}
}

void ModelLoader::Close()
{
}

void ModelLoader::processNode(aiNode* node, const aiScene* scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(this->processMesh(mesh, scene));
	}

	for (UINT i = 0; i < node->mNumChildren; i++) 
	{
		this->processNode(node->mChildren[i], scene);
	}
}

Mesh ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene)
{
	// Data to fill
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
	std::vector<Texture> textures;

	// Walk through each of the mesh's vertices
	for (UINT i = 0; i < mesh->mNumVertices; i++) 
	{
		Vertex vertex;

		vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y,  mesh->mVertices[i].z };
		vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

		if (mesh->mTextureCoords[0]) {
			vertex.texture.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.texture.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		if (face.mNumIndices != 3) continue; // �ﰢ����

		vector<int> currIndices;
		vector<Vector3> pos;
		vector<Vector2> uvs;

		for (UINT j = 0; j < face.mNumIndices; j++)
		{
			int currIndex = face.mIndices[j];
			indices.push_back(currIndex);

			currIndices.push_back(currIndex);
			pos.push_back(vertices[currIndex].position);
			uvs.push_back(vertices[currIndex].texture);
		}

		Vector3 edge1 = pos[1] - pos[0];
		Vector3 edge2 = pos[2] - pos[0];
		Vector2 deltaUV1 = uvs[1] - uvs[0];
		Vector2 deltaUV2 = uvs[2] - uvs[0];

		float det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
		if (fabs(det) < 1e-6f) continue;
		float f = 1.0f / det;

		Vector3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
		Vector3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

		for (int k = 0; k < currIndices.size(); k++)
		{
			int currIndex = currIndices[k];
			vertices[currIndex].tangent += tangent;
			vertices[currIndex].bitangent += bitangent;
		}
	}

	// �������� ����ȭ
	for (auto& v : vertices)
	{
		v.tangent.Normalize();
		v.bitangent.Normalize();
	}

	// ���͸��� �ҷ�����
	if (mesh->mMaterialIndex >= 0) 
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		// diffuseMap �ҷ�����
		std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, TEXTURE_DIFFUSE, scene);
		if (!diffuseMaps.empty())
		{
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		}

		// emissiveMap �ҷ�����
		std::vector<Texture> emissiveMaps = this->loadMaterialTextures(material, aiTextureType_EMISSIVE, TEXTURE_EMISSIVE, scene);
		if (!emissiveMaps.empty())
		{
			textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
		}

		// normalMap �ҷ�����
		std::vector<Texture> normalMaps = this->loadMaterialTextures(material, aiTextureType_NORMALS, TEXTURE_NORMAL, scene);
		if (!normalMaps.empty())
		{
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		}

		// specularMap �ҷ�����
		std::vector<Texture> sepcualrMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, TEXTURE_SPECULAR, scene);
		if (!sepcualrMaps.empty())
		{
			textures.insert(textures.end(), sepcualrMaps.begin(), sepcualrMaps.end());
		}
	}

	return Mesh(m_pDevice, vertices, indices, textures);
}

std::vector<Texture> ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
{
	std::vector<Texture> textures;
	for (UINT i = 0; i < mat->GetTextureCount(type); i++) 
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		for (UINT j = 0; j < texturesLoaded.size(); j++) 
		{
			if (std::strcmp(texturesLoaded[j].path.c_str(), str.C_Str()) == 0) // path Ȯ��
			{
				textures.push_back(texturesLoaded[j]);
				skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}

		if (!skip) 
		{   // If texture hasn't been loaded already, load it
			HRESULT hr;
			Texture texture;

			const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());
			if (embeddedTexture != nullptr) 
			{
				loadEmbeddedTexture(embeddedTexture, texture.pTexture);
			}
			else 
			{
				std::string filename = std::string(str.C_Str());
				filename = directory + '\\' + filename;
				std::wstring filenamews = std::wstring(filename.begin(), filename.end());
				HR_T(CreateWICTextureFromFile(m_pDevice.Get(), m_pDeviceContext.Get(), filenamews.c_str(), nullptr, texture.pTexture.GetAddressOf()));
			}

			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			this->texturesLoaded.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
		}
	}
	return textures;
}

void ModelLoader::loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture)
{
	if (embeddedTexture->mHeight != 0) 
	{
		// Load an uncompressed ARGB8888 embedded texture
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = embeddedTexture->mWidth;
		desc.Height = embeddedTexture->mHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA subresourceData;
		subresourceData.pSysMem = embeddedTexture->pcData;
		subresourceData.SysMemPitch = embeddedTexture->mWidth * 4;
		subresourceData.SysMemSlicePitch = embeddedTexture->mWidth * embeddedTexture->mHeight * 4;

		ID3D11Texture2D* texture2D = nullptr;
		HR_T(m_pDevice->CreateTexture2D(&desc, &subresourceData, &texture2D));

		HR_T(m_pDevice->CreateShaderResourceView(texture2D, nullptr, outTexture.GetAddressOf()));
	}
	else
	{
		// mHeight is 0, so try to load a compressed texture of mWidth bytes
		const size_t size = embeddedTexture->mWidth;

		HR_T(CreateWICTextureFromMemory(m_pDevice.Get(), m_pDeviceContext.Get(), reinterpret_cast<const unsigned char*>(embeddedTexture->pcData), size, nullptr, outTexture.GetAddressOf()));
	}
}
