#pragma once
#include "pch.h"
#include <fbxsdk.h>
#include <string>
#include <map>
#include <vector>

class AnimationStack;


class FBXLoader2 {
public:
	FBXLoader2();
	~FBXLoader2();

	void loadFBXFile(std::string filePath, Mesh::Data& mesh, AnimationStack*& animationStack);

	FbxScene* makeScene(std::string fileName, std::string sceneName);

	void importStaticMesh(FbxScene* scene, Mesh::Data& data);
	void importAnimationStack(AnimationStack* animationStack);

private:
	void fetchGeometry(FbxNode* node, Mesh::Data& mesh);
	void getGeometry(FbxMesh* mesh, Mesh::Data& buildData);
	FbxVector2 getTexCoord(int cpIndex, FbxGeometryElementUV* geUV, FbxMesh* mesh, int polyIndex, int vertIndex) const;

	
	void fetchAnimations(FbxNode* node, AnimationStack* stack);
	void getAnimations(FbxNode* node, AnimationStack* stack);

	void addVertex(Mesh::Data& buildData, unsigned int& uniqueVertices, const unsigned long& currentIndex, const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec2& uv);
	void fetchSkeleton(FbxNode* node, AnimationStack* stack);
	void fetchSkeletonRecursive(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex, AnimationStack* stack);

	int getBoneIndex(unsigned int uniqueID, AnimationStack* stack);

private:
	static FbxManager* s_manager;
	static FbxIOSettings* s_ios;

	//////////////
	std::vector<std::vector<unsigned long>> cpMap;
	FbxScene* fbxScene;
};