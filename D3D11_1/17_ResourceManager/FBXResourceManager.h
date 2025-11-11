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

	bool isRigid = true;
};

class FBXResourceManager
{
	// 해당 매니저에서 fbx를 읽는다.
	// 이미 읽은 fbx는 map에 저장된다.
	std::map<std::string, std::weak_ptr<StaticMeshAsset>> assets;
	
	void ProcessNode(std::shared_ptr<StaticMeshAsset>& pAsset, aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(std::shared_ptr<StaticMeshAsset>& pAsset, aiMesh* mesh, const aiScene* scene);

public:
	std::shared_ptr<StaticMeshAsset> LoadFBXByPath(std::string path);
};
