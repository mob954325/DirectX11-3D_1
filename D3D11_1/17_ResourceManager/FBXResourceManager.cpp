#include "FBXResourceManager.h"

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

void FBXResourceManager::ProcessNode(std::shared_ptr<StaticMeshAsset>& pAsset, aiNode* node, const aiScene* scene)
{
	string boneName = node->mName.C_Str();
	BoneInfo boneInfo = pAsset->skeletalInfo.GetBoneInfoByName(boneName);
	int boneIndex = pAsset->skeletalInfo.GetBoneIndexByName(boneName);

	// node 추적
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		pAsset->meshes.push_back(this->ProcessMesh(pAsset, mesh, scene));
		pAsset->meshes.back().refBoneIndex = boneIndex;

		pAsset->meshes.back().SetMaterial(scene->mMaterials[mesh->mMaterialIndex]);

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

Mesh FBXResourceManager::ProcessMesh(std::shared_ptr<StaticMeshAsset>& pAsset, aiMesh* mesh, const aiScene* scene)
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

std::shared_ptr<StaticMeshAsset> FBXResourceManager::LoadFBXByPath(std::string path)
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

	auto meshAsset = make_shared<StaticMeshAsset>();

	// 리소스 생성하기 
	Assimp::Importer importer;

	unsigned int importFlag = aiProcess_Triangulate |	// 삼각형 변환
		aiProcess_GenNormals |				// 노말 생성
		aiProcess_GenUVCoords |				// UV 생성
		aiProcess_CalcTangentSpace |		// 탄젠트 생성
		aiProcess_LimitBoneWeights |		// 본의 영향을 받는 정점의 최대 개수 4개로 제한
		aiProcess_ConvertToLeftHanded;		// 왼손 좌표계로 변환

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);

	const aiScene* pScene = importer.ReadFile(path, importFlag);

	if (pScene == nullptr)
		return nullptr;

	// check rigid
	for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		if (pScene->mMeshes[i]->mNumBones > 0) // 본이 존재한다 -> skinned
		{
			meshAsset->isRigid = false;
			break;
		}
	}

	// skeletonInfo 저장
	meshAsset->skeletalInfo = SkeletonInfo();
	meshAsset->skeletalInfo.CreateFromAiScene(pScene);

	// animation 저장
	int animationsNum = pScene->mNumAnimations;
	for (int i = 0; i < animationsNum; i++)
	{
		Animation anim;
		anim.CreateBoneAnimation(pScene->mAnimations[i]);
		meshAsset->animations.push_back(anim);
	}

	// mesh 저장

	// texture 저장


	// 없으면 map에 추가 

	return meshAsset;
}
