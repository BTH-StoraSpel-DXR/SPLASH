#include "FBXLoader.h"

#include "Animation.h"

FbxManager* FBXLoader2::s_manager = FbxManager::Create();
FbxIOSettings* FBXLoader2::s_ios = FbxIOSettings::Create(s_manager, IOSROOT);

//transform ripped from https ://github.com/Caspila/GUInity/blob/master/Source/Converter.cpp
static void FBXtoGLM(glm::mat4& newMat, const FbxAMatrix& mat) {
	newMat[0][0] = (float)mat[0][0];
	newMat[0][1] = (float)mat[0][1];
	newMat[0][2] = (float)mat[0][2];
	newMat[0][3] = (float)mat[0][3];
				   
	newMat[1][0] = (float)mat[1][0];
	newMat[1][1] = (float)mat[1][1];
	newMat[1][2] = (float)mat[1][2];
	newMat[1][3] = (float)mat[1][3];
				   
	newMat[2][0] = (float)mat[2][0];
	newMat[2][1] = (float)mat[2][1];
	newMat[2][2] = (float)mat[2][2];
	newMat[2][3] = (float)mat[2][3];
				   
	newMat[3][0] = (float)mat[3][0];
	newMat[3][1] = (float)mat[3][1];
	newMat[3][2] = (float)mat[3][2];
	newMat[3][3] = (float)mat[3][3];
}
static void FBXtoGLM(glm::vec3& newVec, const FbxVector4& vec) {
	newVec.x = (float)vec[0];
	newVec.y = (float)vec[1];
	newVec.z = (float)vec[2];
}

FBXLoader2::FBXLoader2() {
	s_manager->SetIOSettings(s_ios);
	
}
FBXLoader2::~FBXLoader2() {


	
}

void FBXLoader2::loadFBXFile(std::string filePath, Mesh::Data& mesh, AnimationStack*& animationStack) {

	fbxScene = makeScene(filePath, filePath);
	FbxGeometryConverter geoConverter(s_manager);
	if (!geoConverter.Triangulate(fbxScene, true)) {}

	fetchGeometry(fbxScene->GetRootNode(), mesh);
	animationStack = new AnimationStack;
	animationStack->reSizeConnections(mesh.numVertices);

	importAnimationStack(animationStack);

	if (animationStack->getAnimationCount() == 0 && animationStack->boneCount() == 0) {
		delete animationStack;
		animationStack = nullptr;
	}

	fbxScene->Destroy();
	fbxScene = nullptr;
	cpMap.clear();

	return;

}

#pragma region sceneAndDataImporting

FbxScene* FBXLoader2::makeScene(std::string filePath, std::string sceneName) {
	FbxImporter* importer = FbxImporter::Create(s_manager, "");
	if (!importer->Initialize(filePath.c_str(), -1, s_manager->GetIOSettings())) {
		importer->Destroy();
		return nullptr;
	}
	FbxScene* lScene = FbxScene::Create(s_manager, sceneName.c_str());
	static int counter = 0;
	//SAIL_LOG("Trying to import scene "+ sceneName +" : " + std::to_string(counter));
	importer->Import(lScene);
	//SAIL_LOG("imported scene : " + std::to_string(counter++));
	importer->Destroy();

	return lScene;
}



#pragma endregion

#pragma region modelLoading
void FBXLoader2::fetchGeometry(FbxNode* node, Mesh::Data& meshData) {

	unsigned int numAttributes = (unsigned int)node->GetNodeAttributeCount();
	for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; attributeIndex++) {
	
		FbxNodeAttribute* nodeAttribute = node->GetNodeAttributeByIndex(attributeIndex);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();
	
		if (attributeType == FbxNodeAttribute::eMesh) {
			FbxMesh* fbxmesh = node->GetMesh();
			getGeometry(fbxmesh, meshData);
			return;
		}
	}
	
	unsigned int childCount = (unsigned int)node->GetChildCount();
	for (unsigned int child = 0; child < childCount; child++) {
		fetchGeometry(node->GetChild(child), meshData);
	}

}
void FBXLoader2::getGeometry(FbxMesh* mesh, Mesh::Data& buildData) {

	// Number of polygon vertices 
	buildData.numVertices = mesh->GetPolygonVertexCount();
	int* indices = mesh->GetPolygonVertices();

	if (int(buildData.numVertices / 3) != mesh->GetPolygonCount()) {
		return;
	}

	unsigned int uniqueVertices = 0;
	buildData.numIndices = buildData.numVertices;
	buildData.indices = new unsigned long[buildData.numVertices];
	buildData.positions = new glm::vec3[buildData.numVertices];
	buildData.normals = new glm::vec3[buildData.numVertices];
	buildData.texCoords = new glm::vec2[buildData.numVertices];
	buildData.tangents = new glm::vec3[buildData.numVertices];
	buildData.bitangents = new glm::vec3[buildData.numVertices];

	bool norms = true, uvs = true, tangs = true, bitangs = true;

	int vertexIndex = 0;
	FbxVector4* cp = mesh->GetControlPoints();
	cpMap.resize(mesh->GetControlPointsCount());

	glm::vec3 vertPosition[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	glm::vec3 vertNormal[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	glm::vec3 vertTangent[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	glm::vec3 vertBitangent[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	glm::vec2 vertTexCoord[3] = { { 0,0 },{ 0,0 } };
	

	for (unsigned int polyIndex = 0; polyIndex < (unsigned int)mesh->GetPolygonCount(); polyIndex++) {
		unsigned int numVertices = mesh->GetPolygonSize(polyIndex);
		unsigned int CPIndex[3] = { (unsigned int)indices[vertexIndex], (unsigned int)indices[vertexIndex + 1], (unsigned int)indices[vertexIndex + 2] };
		for (unsigned int vertIndex = 0; vertIndex < numVertices; vertIndex += 3) {

#pragma region POSITIONS
			/*
			--	Positions
			*/

			FBXtoGLM(vertPosition[0], cp[indices[vertexIndex]]);
			FBXtoGLM(vertPosition[1], cp[indices[vertexIndex + 1]]);
			FBXtoGLM(vertPosition[2], cp[indices[vertexIndex + 2]]);
#pragma endregion
#pragma region NORMALS
			/*
			--	Normals
			*/
			FbxLayerElementNormal* leNormal = mesh->GetLayer(0)->GetNormals();
			if (leNormal == nullptr && norms) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
				SAIL_LOG_WARNING("Couldn't find any normals in the mesh in the file " + name);
#endif
				norms = false;
			}
			else if (norms) {
				if (leNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
					int normIndex = 0;
					if (leNormal->GetReferenceMode() == FbxLayerElement::eDirect) {
						normIndex = vertexIndex;
					}
					if (leNormal->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
						normIndex = leNormal->GetIndexArray().GetAt(vertexIndex);
					}

					FbxVector4 norm = leNormal->GetDirectArray().GetAt(normIndex);
					FBXtoGLM(vertNormal[0], norm);

					norm = leNormal->GetDirectArray().GetAt(normIndex + 1);
					FBXtoGLM(vertNormal[1], norm);

					norm = leNormal->GetDirectArray().GetAt(normIndex + 2);
					FBXtoGLM(vertNormal[2], norm);
				}
			}
			else {
				vertNormal[0] = { 0,0,0 };
				vertNormal[1] = { 0,0,0 };
				vertNormal[2] = { 0,0,0 };
			}
#pragma endregion
#pragma region TANGENTS
			/*
			--	Tangents
			*/
			FbxGeometryElementTangent* geTang = mesh->GetElementTangent(0);
			if (geTang == nullptr && tangs) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
				SAIL_LOG_WARNING("Couldn't find any tangents in the mesh in the file " + name);
#endif
				tangs = false;
			}
			else if (tangs) {
				if (geTang->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
					int tangIndex = 0;
					if (geTang->GetReferenceMode() == FbxLayerElement::eDirect) {
						tangIndex = vertexIndex;
					}
					if (geTang->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
						tangIndex = geTang->GetIndexArray().GetAt(vertexIndex);
					}

					FbxVector4 tangent = geTang->GetDirectArray().GetAt(tangIndex);
					FBXtoGLM(vertTangent[0], tangent);

					tangent = geTang->GetDirectArray().GetAt(tangIndex + 1);
					FBXtoGLM(vertTangent[1], tangent);

					tangent = geTang->GetDirectArray().GetAt(tangIndex + 2);
					FBXtoGLM(vertTangent[2], tangent);
				}
			}
			else {
				//Keep 
				//vertTangent[0] = glm::normalize(vertPosition[0] - vertPosition[1]);
				//vertTangent[1] = glm::normalize(vertPosition[1] - vertPosition[2]);
				//vertTangent[2] = glm::normalize(vertPosition[2] - vertPosition[0]);

				vertTangent[0] = { 0,0,0 };
				vertTangent[1] = { 0,0,0 };
				vertTangent[2] = { 0,0,0 };

			}
#pragma endregion
#pragma region BINORMALS
			/*
			--	Binormals
			*/
			FbxGeometryElementBinormal* geBN = mesh->GetElementBinormal(0);
			if (geBN == nullptr && bitangs) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
				SAIL_LOG_WARNING("Couldn't find any binormals in the mesh in the file " + name);
#endif
				bitangs = false;
			}
			else if (bitangs) {
				if (geBN->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
					int biNormIndex = 0;
					if (geBN->GetReferenceMode() == FbxLayerElement::eDirect) {
						biNormIndex = vertexIndex;
					}
					if (geBN->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
						biNormIndex = geBN->GetIndexArray().GetAt(vertexIndex);
					}

					FbxVector4 biNorm = geBN->GetDirectArray().GetAt(biNormIndex);
					FBXtoGLM(vertBitangent[0], biNorm);

					biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 1);
					FBXtoGLM(vertBitangent[1], biNorm);

					biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 2);
					FBXtoGLM(vertBitangent[2], biNorm);

				}
			}
			else {

				//Keep 
				//vertBitangent[0] = glm::cross(vertNormal[0], vertTangent[0]);
				//vertBitangent[1] = glm::cross(vertNormal[1], vertTangent[1]);
				//vertBitangent[2] = glm::cross(vertNormal[2], vertTangent[2]);

				vertBitangent[0] = {0,0,0};
				vertBitangent[1] = {0,0,0};
				vertBitangent[2] = {0,0,0};

			}
#pragma endregion
#pragma region TEXCOORDS
			/*
			--	UV Coords
			*/
			FbxGeometryElementUV* geUV = mesh->GetElementUV(0);
			if (geUV == nullptr && uvs) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
				SAIL_LOG_WARNING("Couldn't find any texture coordinates in the mesh in the file " + name);
#endif
				uvs = false;
			}
			else if (uvs) {
				FbxVector2 texCoord;
				int cpIndex;

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex);
				vertTexCoord[0][0] = static_cast<float>(texCoord[0]);
				vertTexCoord[0][1] = -static_cast<float>(texCoord[1]);

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 1);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 1);
				vertTexCoord[1][0] = static_cast<float>(texCoord[0]);
				vertTexCoord[1][1] = -static_cast<float>(texCoord[1]);

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 2);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 2);
				vertTexCoord[2][0] = static_cast<float>(texCoord[0]);
				vertTexCoord[2][1] = -static_cast<float>(texCoord[1]);
			}
			else {
				vertTexCoord[0] = { 0,0 };
				vertTexCoord[1] = { 0,0 };
				vertTexCoord[2] = { 0,0 };
			}
#pragma endregion

			unsigned long oldUnique = uniqueVertices;
			addVertex(buildData, uniqueVertices, vertexIndex++, vertPosition[0], vertNormal[0], vertTangent[0], vertBitangent[0], vertTexCoord[0]);
			
			if (oldUnique != uniqueVertices) {
				cpMap[CPIndex[0]].emplace_back(oldUnique);
			}
			oldUnique = uniqueVertices;
			addVertex(buildData, uniqueVertices, vertexIndex++, vertPosition[1], vertNormal[1], vertTangent[1], vertBitangent[1], vertTexCoord[1]);
			if (oldUnique != uniqueVertices) {
				cpMap[CPIndex[1]].emplace_back(oldUnique);
			}
			oldUnique = uniqueVertices; 
			addVertex(buildData, uniqueVertices, vertexIndex++, vertPosition[2], vertNormal[2], vertTangent[2], vertBitangent[2], vertTexCoord[2]);
			if (oldUnique != uniqueVertices) {
				cpMap[CPIndex[2]].emplace_back(oldUnique);
			}

		}

	}
	buildData.resizeVertices(uniqueVertices);

}
FbxVector2 FBXLoader2::getTexCoord(int cpIndex, FbxGeometryElementUV* geUV, FbxMesh* mesh, int polyIndex, int vertIndex) const {
	FbxVector2 texCoord;

	switch (geUV->GetMappingMode()) {

		// UV per vertex
		case FbxGeometryElement::eByControlPoint:
			switch (geUV->GetReferenceMode()) {
				// Mapped directly per vertex
				case FbxGeometryElement::eDirect:
					texCoord = geUV->GetDirectArray().GetAt(cpIndex);
					break;
				// Mapped by indexed vertex
				case FbxGeometryElement::eIndexToDirect:
					texCoord = geUV->GetDirectArray().GetAt(geUV->GetIndexArray().GetAt(cpIndex));
					break;
				default:
					break;
			}
			break;

		// UV per indexed Vertex
		case FbxGeometryElement::eByPolygonVertex:
			switch (geUV->GetReferenceMode()) {
				// Mapped by indexed vertex
				case FbxGeometryElement::eIndexToDirect:
					texCoord = geUV->GetDirectArray().GetAt(mesh->GetTextureUVIndex(polyIndex, vertIndex));
					break;
				default:
					break;
			}
			break;
	}

	return texCoord;
}

#pragma endregion

#pragma region animationLoading

void FBXLoader2::importStaticMesh(FbxScene* scene, Mesh::Data& data) {
	FbxNode* root = scene->GetRootNode();

	fetchGeometry(root, data);
	//Model* model = new Model(data, shader);

	return;
}

void FBXLoader2::importAnimationStack(AnimationStack* animationStack) {

	fetchSkeleton(fbxScene->GetRootNode(), animationStack);
	fetchAnimations(fbxScene->GetRootNode(), animationStack);
}

void FBXLoader2::fetchAnimations(FbxNode* node, AnimationStack* stack) {

	unsigned int numAttributes =  node->GetNodeAttributeCount();
	for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; attributeIndex++) {

		FbxNodeAttribute* nodeAttribute = node->GetNodeAttributeByIndex(attributeIndex);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();

		if (attributeType == FbxNodeAttribute::eMesh) {
			FbxMesh* fbxmesh = node->GetMesh();
			
			getAnimations(node, stack);
			return;
		}
	}

	unsigned int childCount = (unsigned int)node->GetChildCount();
	for (unsigned int child = 0; child < childCount; child++) {
		fetchAnimations(node->GetChild(child), stack);
	}
}
void FBXLoader2::getAnimations(FbxNode* node, AnimationStack* stack) {

	const FbxScene* scene = node->GetScene();
	std::string nodeName = node->GetName();

	unsigned int numAttributes = node->GetNodeAttributeCount();
	for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; attributeIndex++) {

		
		FbxNodeAttribute* nodeAttribute = node->GetNodeAttributeByIndex(attributeIndex);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();

		if (attributeType == FbxNodeAttribute::eMesh) {
			FbxMesh* mesh = node->GetMesh();
			unsigned int cpCount = mesh->GetControlPointsCount();

			FbxAMatrix geometryTransform(node->GetGeometricTranslation(FbxNode::eSourcePivot), node->GetGeometricRotation(FbxNode::eSourcePivot), node->GetGeometricScaling(FbxNode::eSourcePivot));

			unsigned int deformerCount = mesh->GetDeformerCount();
			for (unsigned int deformerIndex = 0; deformerIndex < deformerCount; deformerIndex++) {
				FbxSkin* skin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
				if (!skin) {
					#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
						SAIL_LOG_ERROR("not a skin at deformer " + std::to_string(deformerIndex) + " in " + nodeName);
					#endif
					continue;
				}
				unsigned int clusterCount = skin->GetClusterCount();

				std::vector<int> limbIndexes;
				limbIndexes.resize(clusterCount);
				for (unsigned int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
					FbxCluster* cluster = skin->GetCluster(clusterIndex);
					limbIndexes[clusterIndex] = getBoneIndex((unsigned int)cluster->GetLink()->GetUniqueID(), stack);
					if (limbIndexes[clusterIndex] == -1) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
						SAIL_LOG_WARNING("Could not find limb at clusterIndex: " + std::to_string(clusterIndex));
#endif
					}
				}

				for (unsigned int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
					FbxCluster* cluster = skin->GetCluster(clusterIndex);
					glm::mat4 globalBindposeInverse;

					FbxAMatrix transformMatrix;
					FbxAMatrix transformLinkMatrix;
					FbxAMatrix globalBindposeInverseMatrix;
					cluster->GetTransformMatrix(transformMatrix);
					cluster->GetTransformLinkMatrix(transformLinkMatrix);
					globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

					FBXtoGLM(globalBindposeInverse, globalBindposeInverseMatrix);

					stack->getBone(limbIndexes[clusterIndex]).globalBindposeInverse = globalBindposeInverse;

					unsigned int indexCount = cluster->GetControlPointIndicesCount();
					int* CPIndices = cluster->GetControlPointIndices();
					double* CPWeights = cluster->GetControlPointWeights();

					for (unsigned int index = 0; index < indexCount; ++index) {

						int indexCP = CPIndices[index];
						int limbIndex = limbIndexes[clusterIndex];
						float limbWeight = (float)CPWeights[index];
						for (int i = 0; i < cpMap[indexCP].size(); i++) {
							unsigned long trueIndex = cpMap[indexCP][i];
							stack->setConnectionData(trueIndex, limbIndex, limbWeight);

						}

					}
				}

				stack->normalizeWeights();
				stack->checkWeights();


				FbxTime::EMode fps = FbxTime::eFrames30;

				Animation* defaultAnimation = new Animation("Default");
				for (unsigned int frameIndex = 0; frameIndex < 3; frameIndex++) {
					Animation::Frame* frame = new Animation::Frame(stack->boneCount());
					glm::mat4 matrix = glm::identity<glm::mat4>();
					FbxTime currTime;
					for (unsigned int boneIndex = 0; boneIndex < clusterCount; boneIndex++) {
						currTime.SetFrame(frameIndex, fps);

						//FbxAMatrix currentTransformOffset = node->EvaluateGlobalTransform(currTime) * geometryTransform;
						//FBXtoGLM(matrix, currentTransformOffset.Inverse() * FbxAMatrix());
						frame->setTransform(limbIndexes[boneIndex], matrix);
					}
					defaultAnimation->addFrame(frameIndex, (float)frameIndex/1.0f, frame);
				}
				stack->addAnimation("Default", defaultAnimation);




				/*  ANIMATION FETCHING FROM STACK*/
				unsigned int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
				// stackCount = 1; // Used for faster debugging
				for (unsigned int currentStack = 0; currentStack < stackCount; currentStack++) {
					FbxAnimStack* currAnimStack = scene->GetSrcObject<FbxAnimStack>(currentStack);
					FbxTakeInfo* takeInfo = scene->GetTakeInfo(currAnimStack->GetName());
					std::string animationName = currAnimStack->GetName();
					Animation* animation = new Animation(animationName);
					node->GetScene()->SetCurrentAnimationStack(currAnimStack);

					FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
					FbxTime end = takeInfo->mLocalTimeSpan.GetStop();

					float firstFrameTime = 0.0f;

					//TODO: find way to import FPS from file.
					bool firstFrame = true;
					unsigned int offset = 0;
					for (FbxLongLong frameIndex = start.GetFrameCount(fps); frameIndex <= end.GetFrameCount(fps); frameIndex++) {
						Animation::Frame* frame = new Animation::Frame(stack->boneCount());
						FbxTime currTime;
						currTime.SetFrame(frameIndex, fps);
						if (firstFrame) {
							offset = frameIndex;
							firstFrame = false;
						}

						if (firstFrameTime == 0.0f) {
							firstFrameTime = float(currTime.GetSecondDouble());
						}
						glm::mat4 matrix;		
						
						for (unsigned int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex) {
							FbxCluster* cluster = skin->GetCluster(clusterIndex);

							FbxAMatrix currentTransformOffset = node->EvaluateGlobalTransform(currTime) * geometryTransform;

							FBXtoGLM(matrix, currentTransformOffset.Inverse() * cluster->GetLink()->EvaluateGlobalTransform(currTime));

							frame->setTransform(limbIndexes[clusterIndex],  matrix * stack->getBone(limbIndexes[clusterIndex]).globalBindposeInverse);
						}
						float time = float(currTime.GetSecondDouble()) - firstFrameTime;
						animation->addFrame(frameIndex - offset, time, frame);
					}
					
					stack->addAnimation(animationName, animation);
				}
			}//deformer end
		}
		
	} // attribute end
}
#pragma endregion

void FBXLoader2::addVertex(Mesh::Data& buildData, unsigned int& uniqueVertices, const unsigned long& currentIndex, const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec2& uv) {
	for (unsigned int vert = 0; vert < uniqueVertices; vert++) {
		if (buildData.positions[vert] == position && buildData.normals[vert] == normal && buildData.texCoords[vert] == uv) {
			buildData.indices[currentIndex] = vert;
			return;
		}
	}

	buildData.positions[uniqueVertices] = position;
	buildData.normals[uniqueVertices] = normal;
	buildData.tangents[uniqueVertices] = tangent;
	buildData.bitangents[uniqueVertices] = bitangent;
	buildData.texCoords[uniqueVertices] = uv;
	buildData.indices[currentIndex] = uniqueVertices;
	uniqueVertices += 1;
}
void FBXLoader2::fetchSkeleton(FbxNode* node, AnimationStack* stack) {
	for (int childIndex = 0; childIndex < node->GetChildCount(); ++childIndex) {
		FbxNode* currNode = node->GetChild(childIndex);
		fetchSkeletonRecursive(currNode, 0, 0, -1, stack);
	}

}
void FBXLoader2::fetchSkeletonRecursive(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex, AnimationStack* stack) {

	if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
		AnimationStack::Bone limb;
		limb.parentIndex = inParentIndex;
		limb.uniqueID = inNode->GetUniqueID();
		limb.name = inNode->GetName();

		if (stack->boneCount() > 0) {
			stack->getBone(limb.parentIndex).childIndexes.emplace_back(stack->boneCount());
		}
		stack->addBone(limb);

	}
	for (int i = 0; i < inNode->GetChildCount(); i++) {
		fetchSkeletonRecursive(inNode->GetChild(i), inDepth + 1, stack->boneCount(), myIndex, stack);
	}


}
int FBXLoader2::getBoneIndex(unsigned int uniqueID, AnimationStack* stack) {
	unsigned int size = stack->boneCount();
	for (unsigned int i = 0; i < size; i++) {
		if (stack->getBone(i).uniqueID == uniqueID) {
			return i;
		}
	}
	return -1;
}