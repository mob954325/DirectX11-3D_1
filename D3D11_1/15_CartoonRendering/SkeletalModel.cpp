#include "SkeletalModel.h"
#include "../Common/Helper.h"
#include "../Common/TimeSystem.h"

/// <summary>
/// 모델에서 사용할 트랜스폼 상수 버퍼 구조체
/// </summary>
struct TransformBuffer
{
	Matrix world;

	UINT isRigid;		// 1 : rigid, 0 : skinned
	UINT refBoneIndex;	// 리지드일 때 참조하는 본 인덱스
	FLOAT pad1;
	FLOAT pad2;
};

SkeletalModel::SkeletalModel()
{
}

SkeletalModel::~SkeletalModel()
{
}

bool SkeletalModel::Load(HWND hwnd, ComPtr<ID3D11Device>& pDevice, ComPtr<ID3D11DeviceContext>& pDeviceContext, std::string filename)
{
	LoadRampTexture(pDevice);

	Assimp::Importer importer;

	unsigned int importFlag = aiProcess_Triangulate |	// 삼각형 변환
		aiProcess_GenNormals |				// 노말 생성
		aiProcess_GenUVCoords |				// UV 생성
		aiProcess_CalcTangentSpace |		// 탄젠트 생성
		aiProcess_LimitBoneWeights |		// 본의 영향을 받는 정점의 최대 개수 4개로 제한
		aiProcess_ConvertToLeftHanded;		// 왼손 좌표계로 변환

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
	
	const aiScene* pScene = importer.ReadFile(filename, importFlag);

	if (pScene == nullptr)
		return false;	

	// check rigid
	for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		if (pScene->mMeshes[i]->mNumBones > 0) // 본이 존재한다 -> skinned
		{
			isRigid = false;
			break;
		}
	}

	this->directory = filename.substr(0, filename.find_last_of("/\\"));
	this->m_pDevice = pDevice;
	this->m_pDeviceContext = pDeviceContext;
	this->hwnd = hwnd;

	// 트랜스폼 상수 버퍼 만들기
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(TransformBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pTransformBuffer.GetAddressOf()));

	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BonePoseBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pBonePoseBuffer.GetAddressOf()));

	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BoneOffsetBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pBoneOffsetBuffer.GetAddressOf()));

	// skeletonInfo 저장
	m_pSkeletonInfo = make_shared<SkeletonInfo>();
	m_pSkeletonInfo->CreateFromAiScene(pScene);

	// animations -> mchannels -> mxxxKeys
	int animationsNum = pScene->mNumAnimations;
	for (int i = 0; i < animationsNum; i++)
	{
		Animation anim;
		anim.CreateBoneAnimation(pScene->mAnimations[i]);
		m_animations.push_back(anim);		
	}

	// 노드 별로 모델 갱신
	ProcessNode(pScene->mRootNode, pScene);

	// 정점 버퍼, 인덱스 버퍼 생성
	for (auto& mesh : m_meshes)
	{
		mesh.CreateVertexBuffer(pDevice);
		mesh.CreateIndexBuffer(pDevice);
	}

	return true;
}

void SkeletalModel::Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext, ComPtr<ID3D11Buffer>& pMatBuffer)
{
	m_pDeviceContext->PSSetShaderResources(4, 1, diffuseRamp.pTexture.GetAddressOf());
	m_pDeviceContext->PSSetShaderResources(5, 1, specRamp.pTexture.GetAddressOf());

	m_pDeviceContext->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, &m_BonePoses, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pBoneOffsetBuffer.Get(), 0, nullptr, &m_BoneOffsets, 0, 0);

	m_pDeviceContext->VSSetConstantBuffers(3, 1, m_pBonePoseBuffer.GetAddressOf());
	m_pDeviceContext->VSSetConstantBuffers(4, 1, m_pBoneOffsetBuffer.GetAddressOf());

	TransformBuffer tb = {};

	m_world = m_world.CreateScale(m_Scale) *
			  m_world.CreateFromYawPitchRoll(m_Rotation) *
			  m_world.CreateTranslation(m_Position);
	tb.world = XMMatrixTranspose(m_world);

	int size = m_meshes.size();
	for (size_t i = 0; i < size; i++)
	{
		Material meshMaterial = m_meshes[i].GetMaterial();
		m_pDeviceContext->UpdateSubresource(pMatBuffer.Get(), 0, nullptr, &meshMaterial, 0, 0);		

		tb.refBoneIndex = m_meshes[i].refBoneIndex;
		
		m_pDeviceContext->UpdateSubresource(m_pTransformBuffer.Get(), 0, nullptr, &tb, 0, 0);
		m_pDeviceContext->VSSetConstantBuffers(2, 1, m_pTransformBuffer.GetAddressOf());
		m_pDeviceContext->PSSetConstantBuffers(1, 1, pMatBuffer.GetAddressOf());

		m_meshes[i].Draw(pDeviceContext);
	}
}

void SkeletalModel::Update()
{
	if (!m_animations.empty() && isAnimPlay)
	{
		m_progressAnimationTime += GameTimer::m_Instance->DeltaTime();
		m_progressAnimationTime = fmod(m_progressAnimationTime, m_animations[m_animationIndex].m_duration);	
	}

	// pose 본 갱신
	for (int i = 0; i < m_bones.size(); i++)
	{
		// 애니메이션 업데이트
		if (m_bones[i].m_boneAnimation.m_boneName != "")
		{
			Vector3 positionVec, scaleVec;
			Quaternion rotationQuat;
			m_bones[i].m_boneAnimation.Evaluate(m_progressAnimationTime, positionVec, rotationQuat, scaleVec);

			Matrix mat = Matrix::CreateScale(scaleVec) * Matrix::CreateFromQuaternion(rotationQuat) * Matrix::CreateTranslation(positionVec);
			m_bones[i].m_localTransform = mat.Transpose();
		}

		// 위치 갱신
		if (m_bones[i].m_parentIndex != -1)
		{
			m_bones[i].m_worldTransform = m_bones[m_bones[i].m_parentIndex].m_worldTransform * m_bones[i].m_localTransform;
		}
		else
		{
			m_bones[i].m_worldTransform = m_bones[i].m_localTransform;
		}

		m_BonePoses.modelMatricies[m_bones[i].m_index] = m_bones[i].m_worldTransform;
	
		// update offsetMatrix
		Matrix offsetMat = Matrix::Identity;
		if (i > 0)
		{
			offsetMat = m_pSkeletonInfo->GetBoneOffsetByName(m_bones[i].name);
		}
		m_BoneOffsets.boneOffset[m_bones[i].m_index] = offsetMat;
	}		
}

void SkeletalModel::Close()
{
}

void SkeletalModel::ProcessNode(aiNode* node, const aiScene* scene)
{
	// Bone 정보 생성
	Bone bone;

	string boneName = node->mName.C_Str();
	BoneInfo boneInfo = m_pSkeletonInfo->GetBoneInfoByName(boneName);
	int boneIndex = m_pSkeletonInfo->GetBoneIndexByName(boneName);

	string parentBoneName = boneInfo.parentBoneName;
	BoneInfo parentBoneInfo;
	int parentBoneIndex = -1;
	if (parentBoneName != "")
	{
		parentBoneInfo = m_pSkeletonInfo->GetBoneInfoByName(parentBoneName);
		parentBoneIndex = m_pSkeletonInfo->GetBoneIndexByName(parentBoneName);
	}

	Matrix localMat = boneInfo.relativeTransform;
	Matrix worldMat = parentBoneIndex > 0 ? m_bones[parentBoneIndex].m_worldTransform * localMat : localMat;

	bone.CreateBone(boneName, parentBoneIndex, boneIndex, worldMat, localMat);	//...

	BoneAnimation boneAnim;
	bool hasAnim = scene->mNumAnimations > 0;
	if(parentBoneIndex != -1 && hasAnim)
	{
		m_animations[m_animationIndex].GetBoneAnimationByName(boneName, boneAnim);
		bone.m_boneAnimation = boneAnim;	// 임시 -> 0번째 애니메이션 받기
	}

	m_bones.push_back(bone);

	// node 추적
	for (UINT i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_meshes.push_back(this->ProcessMesh(mesh, scene));
		m_meshes.back().refBoneIndex = boneIndex;
		
		m_meshes.back().SetMaterial(scene->mMaterials[mesh->mMaterialIndex]);

		// weight 저장
		if (!isRigid)
		{
			ProcessBoneWeight(mesh);
		}
	}

	for (UINT i = 0; i < node->mNumChildren; i++) 
	{
		this->ProcessNode(node->mChildren[i], scene);
	}
}

Mesh SkeletalModel::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	// Data to fill
	std::vector<BoneWeightVertex> vertices;
	std::vector<UINT> indices;
	std::vector<Texture> textures;

	// Walk through each of the mesh's vertices
	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		BoneWeightVertex vertex{};

		vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y,  mesh->mVertices[i].z };
		vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
		vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };

		if (mesh->mTextureCoords[0]) {
			vertex.texture.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.texture.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);
	}

	// face 인덱스 저장
	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		if (face.mNumIndices != 3) continue; // 삼각형만

		for (UINT j = 0; j < face.mNumIndices; j++)
		{
			int currIndex = face.mIndices[j];
			indices.push_back(currIndex);
		}
	}

	// 머터리얼 불러오기
	if (mesh->mMaterialIndex >= 0) 
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		// diffuseMap 불러오기
		std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, TEXTURE_DIFFUSE, scene);
		if (!diffuseMaps.empty())
		{
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		}

		// emissiveMap 불러오기
		std::vector<Texture> emissiveMaps = this->loadMaterialTextures(material, aiTextureType_EMISSIVE, TEXTURE_EMISSIVE, scene);
		if (!emissiveMaps.empty())
		{
			textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
		}

		// normalMap 불러오기
		std::vector<Texture> normalMaps = this->loadMaterialTextures(material, aiTextureType_NORMALS, TEXTURE_NORMAL, scene);
		if (!normalMaps.empty())
		{
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		}

		// specularMap 불러오기
		std::vector<Texture> sepcualrMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, TEXTURE_SPECULAR, scene);
		if (!sepcualrMaps.empty())
		{
			textures.insert(textures.end(), sepcualrMaps.begin(), sepcualrMaps.end());
		}
	}

	return Mesh(m_pDevice, vertices, indices, textures);
}

void SkeletalModel::ProcessBoneWeight(aiMesh* pMesh)
{
	UINT meshBoneCount = pMesh->mNumBones;
	for (UINT i = 0; i < meshBoneCount; i++)
	{
		aiBone* pAiBone = pMesh->mBones[i];
		UINT boneIndex = m_pSkeletonInfo->GetBoneIndexByName(pAiBone->mName.C_Str());
		BoneInfo bone = m_pSkeletonInfo->GetBoneInfoByIndex(boneIndex);

		for (UINT j = 0; j < pAiBone->mNumWeights; j++)
		{
			UINT vertexId = pAiBone->mWeights[j].mVertexId;
			float weight = pAiBone->mWeights[j].mWeight;
			m_meshes.back().vertices[vertexId].AddBoneData(boneIndex, weight);
		}
	}
}

std::vector<Texture> SkeletalModel::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
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
			if (std::strcmp(texturesLoaded[j].path.c_str(), str.C_Str()) == 0) // path 확인
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

void SkeletalModel::loadEmbeddedTexture(const aiTexture* embeddedTexture, ComPtr<ID3D11ShaderResourceView>& outTexture)
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

void SkeletalModel::LoadRampTexture(ComPtr<ID3D11Device> pDevice)
{
	HR_T(CreateTextureFromFile(pDevice.Get(), L"..\\Resource\\RampTexture.png", diffuseRamp.pTexture.GetAddressOf()));
	diffuseRamp.path = "..\\Resoure\\RampTexture.png";
	diffuseRamp.type = "DiffuseRamp";

	HR_T(CreateTextureFromFile(pDevice.Get(), L"..\\Resource\\SpecularRampTexture.png", specRamp.pTexture.GetAddressOf()));
	specRamp.path = "..\\Resoure\\SpecularRampTexture.png";
	specRamp.type = "SpecularRamp";
}