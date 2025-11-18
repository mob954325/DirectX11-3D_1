#include "FBXResourceManager.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

void FBXResourceManager::ProcessNode(std::shared_ptr<StaticMeshAsset>& pAsset, aiNode* pNode, const aiScene* pScene)
{
	string boneName = pNode->mName.C_Str();
	BoneInfo boneInfo = pAsset->skeletalInfo.GetBoneInfoByName(boneName);
	int boneIndex = pAsset->skeletalInfo.GetBoneIndexByName(boneName);

	// node 추적
	for (UINT i = 0; i < pNode->mNumMeshes; i++)
	{
		aiMesh* mesh = pScene->mMeshes[pNode->mMeshes[i]];
		pAsset->meshes.push_back(this->ProcessMesh(pAsset, mesh, pScene));
		pAsset->meshes.back().refBoneIndex = boneIndex;

		pAsset->meshes.back().SetMaterial(pScene->mMaterials[mesh->mMaterialIndex]);

		// weight 저장
		ProcessBoneWeight(pAsset, mesh);
	}

	for (UINT i = 0; i < pNode->mNumChildren; i++)
	{
		this->ProcessNode(pAsset, pNode->mChildren[i], pScene);
	}
}

Mesh FBXResourceManager::ProcessMesh(std::shared_ptr<StaticMeshAsset>& pAsset, aiMesh* pMesh, const aiScene* pScene)
{
	// Data to fill
	std::vector<BoneWeightVertex> vertices;
	std::vector<UINT> indices;
	std::vector<Texture> textures;

	// Walk through each of the mesh's vertices
	for (UINT i = 0; i < pMesh->mNumVertices; i++)
	{
		BoneWeightVertex vertex{};

		vertex.position = { pMesh->mVertices[i].x, pMesh->mVertices[i].y,  pMesh->mVertices[i].z };
		vertex.normal = { pMesh->mNormals[i].x, pMesh->mNormals[i].y, pMesh->mNormals[i].z };
		vertex.tangent = { pMesh->mTangents[i].x, pMesh->mTangents[i].y, pMesh->mTangents[i].z };
		vertex.bitangent = { pMesh->mBitangents[i].x, pMesh->mBitangents[i].y, pMesh->mBitangents[i].z };

		if (pMesh->mTextureCoords[0]) {
			vertex.texture.x = (float)pMesh->mTextureCoords[0][i].x;
			vertex.texture.y = (float)pMesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);
	}

	// face 인덱스 저장
	for (UINT i = 0; i < pMesh->mNumFaces; i++)
	{
		aiFace face = pMesh->mFaces[i];
		if (face.mNumIndices != 3) continue; // 삼각형만

		for (UINT j = 0; j < face.mNumIndices; j++)
		{
			int currIndex = face.mIndices[j];
			indices.push_back(currIndex);
		}
	}

	// 머터리얼 불러오기
	if (pMesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = pScene->mMaterials[pMesh->mMaterialIndex];

		// diffuseMap 불러오기
		std::vector<Texture> diffuseMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_DIFFUSE, TEXTURE_DIFFUSE, pScene);
		if (!diffuseMaps.empty())
		{
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		}

		// emissiveMap 불러오기
		std::vector<Texture> emissiveMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_EMISSIVE, TEXTURE_EMISSIVE, pScene);
		if (!emissiveMaps.empty())
		{
			textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
		}

		// normalMap 불러오기
		std::vector<Texture> normalMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_NORMALS, TEXTURE_NORMAL, pScene);
		if (!normalMaps.empty())
		{
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		}

		// specularMap 불러오기
		std::vector<Texture> sepcualrMaps = this->loadMaterialTextures(pAsset, material, aiTextureType_SPECULAR, TEXTURE_SPECULAR, pScene);
		if (!sepcualrMaps.empty())
		{
			textures.insert(textures.end(), sepcualrMaps.begin(), sepcualrMaps.end());
		}
	}

	return Mesh(m_pDevice, vertices, indices, textures);
}

void FBXResourceManager::ProcessBoneWeight(std::shared_ptr<StaticMeshAsset>& pAsset, aiMesh* pMesh)
{
	UINT meshBoneCount = pMesh->mNumBones;
	if (meshBoneCount <= 0) return;

	for (UINT i = 0; i < meshBoneCount; i++)
	{
		aiBone* pAiBone = pMesh->mBones[i];
		UINT boneIndex = pAsset->skeletalInfo.GetBoneIndexByName(pAiBone->mName.C_Str());
		BoneInfo bone = pAsset->skeletalInfo.GetBoneInfoByIndex(boneIndex);

		for (UINT j = 0; j < pAiBone->mNumWeights; j++)
		{
			UINT vertexId = pAiBone->mWeights[j].mVertexId;
			float weight = pAiBone->mWeights[j].mWeight;
			pAsset->meshes.back().vertices[vertexId].AddBoneData(boneIndex, weight);
		}
	}
}

std::vector<Texture> FBXResourceManager::loadMaterialTextures(std::shared_ptr<StaticMeshAsset>& pAsset, aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
{
	std::vector<Texture> textures;
	for (UINT i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		auto textureloadeds = pAsset->textures;
		for (UINT j = 0; j < textureloadeds.size(); j++)
		{
			if (std::strcmp(textureloadeds[j].path.c_str(), str.C_Str()) == 0) // path 확인
			{
				textures.push_back(textureloadeds[j]);
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
				filename = pAsset->directory + '\\' + filename;
				std::wstring filenamews = std::wstring(filename.begin(), filename.end());
				HR_T(CreateWICTextureFromFile(m_pDevice.Get(), m_pDeviceContext.Get(), filenamews.c_str(), nullptr, texture.pTexture.GetAddressOf()));
			}

			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textureloadeds.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
		}
	}
	return textures;
}

void FBXResourceManager::loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture)
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

std::shared_ptr<StaticMeshAsset> FBXResourceManager::LoadFBXByPath(ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string path)
{
	// map에 먼저 있는지 확인
	auto it = assets.find(path);

	if (it != assets.end()) // 존재함
	{
		if (!it->second.expired())
		{
			shared_ptr<StaticMeshAsset> assetPtr = it->second.lock();
			return assetPtr;
		}
		else
		{
			assets.erase(it); // 지우고 새로 만들기
		}
	}

	Assimp::Importer importer;

	unsigned int importFlag = aiProcess_Triangulate |	// 삼각형 변환
		aiProcess_GenNormals |				// 노말 생성
		aiProcess_GenUVCoords |				// UV 생성
		aiProcess_CalcTangentSpace |		// 탄젠트 생성
		aiProcess_LimitBoneWeights |		// 본의 영향을 받는 정점의 최대 개수 4개로 제한
		aiProcess_ConvertToLeftHanded;		// 왼손 좌표계로 변환

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);

	const aiScene* pScene = importer.ReadFile(path, importFlag);

	if (pScene == nullptr) return std::shared_ptr<StaticMeshAsset>();

	this->m_pDevice = pDevice;
	this->m_pDeviceContext = pDeviceContext;

	// 없으면 load후 map에 추가 
	auto sharedAsset = make_shared<StaticMeshAsset>();

	if (pScene == nullptr)
		return nullptr;

	sharedAsset ->directory = path.substr(0, path.find_last_of("/\\"));

	// skeletonInfo 저장
	sharedAsset ->skeletalInfo = SkeletonInfo();
	sharedAsset ->skeletalInfo.CreateFromAiScene(pScene);

	// animation 저장
	int animationsNum = pScene->mNumAnimations;
	for (int i = 0; i < animationsNum; i++)
	{
		Animation anim;
		anim.CreateBoneAnimation(pScene->mAnimations[i]);
		sharedAsset->animations.push_back(anim);
	}

	// mesh 저장, texture 저장 
	ProcessNode(sharedAsset , pScene->mRootNode, pScene); // 내부에서 mesh의 텍스처 저장함

	// mesh의 정점 버퍼, 인덱스 버퍼 생성
	for (auto& mesh : sharedAsset->meshes)
	{
		mesh.CreateVertexBuffer(pDevice);
		mesh.CreateIndexBuffer(pDevice);
	}

	// bone offest 버퍼 채우기
	for (auto& bone : sharedAsset->skeletalInfo.m_bones)
	{
		Matrix offsetMat = Matrix::Identity;
		std::string currBoneName = bone.name;
		int boneIndex = sharedAsset->skeletalInfo.GetBoneIndexByName(currBoneName);

		if (boneIndex > 0)
		{
			offsetMat = sharedAsset->skeletalInfo.GetBoneOffsetByName(currBoneName);
		}

		sharedAsset->m_BoneOffsets.boneOffset[boneIndex] = offsetMat;
	}

	// map에 저장하기
	weak_ptr<StaticMeshAsset> weakAsset = sharedAsset;
	assets.insert({ path, weakAsset });

	return sharedAsset;
}
