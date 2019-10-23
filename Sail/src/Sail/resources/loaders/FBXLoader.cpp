#include "pch.h"
#include "FBXLoader.h"

#include <d3d11.h>
#include "../../utils/Utils.h"
#include "Sail/Application.h"
#include "Sail/graphics/geometry/Animation.h"

FbxManager* FBXLoader::s_manager = FbxManager::Create();
FbxIOSettings* FBXLoader::s_ios = FbxIOSettings::Create(s_manager, IOSROOT);

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

FBXLoader::FBXLoader() {
	s_manager->SetIOSettings(s_ios);
	
}
FBXLoader::~FBXLoader() {


	
}

#pragma region sceneAndDataImporting
bool FBXLoader::importScene(const std::string& filePath, Shader* shader) {
	if (m_sceneData.find(filePath) != m_sceneData.end()) {
		return true;
	}
	if (!initScene(filePath)) {
		return false;
	}

	if (m_scenes.find(filePath) == m_scenes.end()) {
		return false;
	}
	const FbxScene* scene = m_scenes[filePath];
	assert(scene);


	Model* model = importStaticModel(filePath, shader);
	if (model) {
		m_sceneData[filePath].hasModel = true;
		m_sceneData[filePath].model = model;
		//here would getMaterial for model be
	}

	m_sceneData[filePath].stack = importAnimationStack(filePath);
	if (m_sceneData[filePath].stack) {
		m_sceneData[filePath].hasAnimation = true;
	}

	return true;
}
bool FBXLoader::initScene(const std::string& filePath) {
	if (m_scenes.find(filePath) == m_scenes.end()) {
		FbxScene* scene = makeScene(filePath, filePath);

		if (!scene) {
			return false;
		}
		FbxGeometryConverter geoConverter(s_manager);
		if (!geoConverter.Triangulate(scene, true)) {
			assert(scene);
		}
		
		m_scenes[filePath] = scene;
		m_sceneData[filePath] = { false, false, false, false, nullptr, nullptr}; //??
	}
	return true;
}
FbxScene* FBXLoader::makeScene(std::string filePath, std::string sceneName) {
	FbxImporter* importer = FbxImporter::Create(s_manager, "");
	if (!importer->Initialize(filePath.c_str(), -1, s_manager->GetIOSettings())) {
		importer->Destroy();
#if _DEBUG
		Logger::Error("Could not load (" + filePath + ")");
		std::cout << "Could not Load '" + filePath + "'" << std::endl;
		assert(0);
#endif
		return nullptr;
	}
	FbxScene* lScene = FbxScene::Create(s_manager, sceneName.c_str());


	importer->Import(lScene);
	importer->Destroy();

	return lScene;
}
void FBXLoader::clearScene(const std::string& filePath) {
	m_scenes[filePath];
	m_scenes.erase(filePath);
}
void FBXLoader::clearAllScenes() {
	//todo?	
}
#pragma endregion

#pragma region modelLoading
Model* FBXLoader::importStaticModel(const std::string& filePath, Shader* shader) {
	const FbxScene* scene = m_scenes[filePath];
	assert(scene);

	FbxNode* root = scene->GetRootNode();
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
		printNodeTree(root, "");
#endif

	// LOAD MODEL
	Mesh::Data data;
	fetchGeometry(root, data, filePath);
	Model* model = SAIL_NEW Model(data, shader);

	return model;
}
void FBXLoader::fetchGeometry(FbxNode* node, Mesh::Data& meshData, const std::string& name) {

	FbxScene* scene = node->GetScene();
	unsigned int numAttributes = (unsigned int)node->GetNodeAttributeCount();
	for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; attributeIndex++) {
	
		FbxNodeAttribute* nodeAttribute = node->GetNodeAttributeByIndex(attributeIndex);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();
	
		if (attributeType == FbxNodeAttribute::eMesh) {
			FbxMesh* fbxmesh = node->GetMesh();
			getGeometry(fbxmesh, meshData, name);
			return;
		}
	}
	
	unsigned int childCount = (unsigned int)node->GetChildCount();
	for (unsigned int child = 0; child < childCount; child++) {
		fetchGeometry(node->GetChild(child), meshData, name);
	}

}
void FBXLoader::getGeometry(FbxMesh* mesh, Mesh::Data& buildData, const std::string& name) {

	// Number of polygon vertices 
	buildData.numVertices = mesh->GetPolygonVertexCount();
	int* indices = mesh->GetPolygonVertices();

	if (int(buildData.numVertices / 3) != mesh->GetPolygonCount()) {
		Logger::Error("The mesh in '" + name + "' has to be triangulated.");
		return;
	}

	unsigned int uniqueVertices = 0;
	buildData.numIndices = buildData.numVertices;
	buildData.indices = SAIL_NEW unsigned long[buildData.numVertices];
	buildData.positions = SAIL_NEW Mesh::vec3[buildData.numVertices];
	buildData.normals = SAIL_NEW Mesh::vec3[buildData.numVertices];
	buildData.texCoords = SAIL_NEW Mesh::vec2[buildData.numVertices];
	buildData.tangents = SAIL_NEW Mesh::vec3[buildData.numVertices];
	buildData.bitangents = SAIL_NEW Mesh::vec3[buildData.numVertices];

	bool norms = true, uvs = true, tangs = true, bitangs = true;

	int vertexIndex = 0;
	FbxVector4* cp = mesh->GetControlPoints();
	auto& cpMap = m_sceneData[name].cpToVertMap;
	cpMap.resize(mesh->GetControlPointsCount());
	if (cp == nullptr) {

		Logger::Error("Couldn't find any vertices in the mesh in the file " + name);
		return;
	}

	Mesh::vec3 vertPosition[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	Mesh::vec3 vertNormal[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	Mesh::vec3 vertTangent[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	Mesh::vec3 vertBitangent[3] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
	Mesh::vec2 vertTexCoord[3] = { { 0,0 },{ 0,0 } };
	



	for (unsigned int polyIndex = 0; polyIndex < (unsigned int)mesh->GetPolygonCount(); polyIndex++) {
		unsigned int numVertices = mesh->GetPolygonSize(polyIndex);
		unsigned int CPIndex[3] = { (unsigned int)indices[vertexIndex], (unsigned int)indices[vertexIndex + 1], (unsigned int)indices[vertexIndex + 2] };
		for (unsigned int vertIndex = 0; vertIndex < numVertices; vertIndex += 3) {

#pragma region POSITIONS
			/*
			--	Positions
			*/

			FBXtoGLM(vertPosition[0].vec, cp[indices[vertexIndex]]);
			FBXtoGLM(vertPosition[1].vec, cp[indices[vertexIndex + 1]]);
			FBXtoGLM(vertPosition[2].vec, cp[indices[vertexIndex + 2]]);
#pragma endregion
#pragma region NORMALS
			/*
			--	Normals
			*/
			FbxLayerElementNormal* leNormal = mesh->GetLayer(0)->GetNormals();
			if (leNormal == nullptr && norms) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
				Logger::Warning("Couldn't find any normals in the mesh in the file " + name);
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
					FBXtoGLM(vertNormal[0].vec, norm);

					norm = leNormal->GetDirectArray().GetAt(normIndex + 1);
					FBXtoGLM(vertNormal[1].vec, norm);

					norm = leNormal->GetDirectArray().GetAt(normIndex + 2);
					FBXtoGLM(vertNormal[2].vec, norm);
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
				Logger::Warning("Couldn't find any tangents in the mesh in the file " + name);
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
					FBXtoGLM(vertTangent[0].vec, tangent);

					tangent = geTang->GetDirectArray().GetAt(tangIndex + 1);
					FBXtoGLM(vertTangent[1].vec, tangent);

					tangent = geTang->GetDirectArray().GetAt(tangIndex + 2);
					FBXtoGLM(vertTangent[2].vec, tangent);
				}
			}
			else {
				//Keep 
				//vertTangent[0].vec = glm::normalize(vertPosition[0].vec - vertPosition[1].vec);
				//vertTangent[1].vec = glm::normalize(vertPosition[1].vec - vertPosition[2].vec);
				//vertTangent[2].vec = glm::normalize(vertPosition[2].vec - vertPosition[0].vec);

				vertTangent[0].vec = { 0,0,0 };
				vertTangent[1].vec = { 0,0,0 };
				vertTangent[2].vec = { 0,0,0 };

			}
#pragma endregion
#pragma region BINORMALS
			/*
			--	Binormals
			*/
			FbxGeometryElementBinormal* geBN = mesh->GetElementBinormal(0);
			if (geBN == nullptr && bitangs) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
				Logger::Warning("Couldn't find any binormals in the mesh in the file " + name);
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
					FBXtoGLM(vertBitangent[0].vec, biNorm);

					biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 1);
					FBXtoGLM(vertBitangent[1].vec, biNorm);

					biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 2);
					FBXtoGLM(vertBitangent[2].vec, biNorm);

				}
			}
			else {

				//Keep 
				//vertBitangent[0].vec = glm::cross(vertNormal[0].vec, vertTangent[0].vec);
				//vertBitangent[1].vec = glm::cross(vertNormal[1].vec, vertTangent[1].vec);
				//vertBitangent[2].vec = glm::cross(vertNormal[2].vec, vertTangent[2].vec);

				vertBitangent[0].vec = {0,0,0};
				vertBitangent[1].vec = {0,0,0};
				vertBitangent[2].vec = {0,0,0};

			}
#pragma endregion
#pragma region TEXCOORDS
			/*
			--	UV Coords
			*/
			FbxGeometryElementUV* geUV = mesh->GetElementUV(0);
			if (geUV == nullptr && uvs) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
				Logger::Warning("Couldn't find any texture coordinates in the mesh in the file " + name);
#endif
				uvs = false;
			}
			else if (uvs) {
				FbxVector2 texCoord;
				int cpIndex;

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex);
				vertTexCoord[0].vec[0] = static_cast<float>(texCoord[0]);
				vertTexCoord[0].vec[1] = -static_cast<float>(texCoord[1]);

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 1);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 1);
				vertTexCoord[1].vec[0] = static_cast<float>(texCoord[0]);
				vertTexCoord[1].vec[1] = -static_cast<float>(texCoord[1]);

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 2);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 2);
				vertTexCoord[2].vec[0] = static_cast<float>(texCoord[0]);
				vertTexCoord[2].vec[1] = -static_cast<float>(texCoord[1]);
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
FbxVector2 FBXLoader::getTexCoord(int cpIndex, FbxGeometryElementUV* geUV, FbxMesh* mesh, int polyIndex, int vertIndex) const {
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
AnimationStack* FBXLoader::importAnimationStack(const std::string& filePath) {
	const FbxScene* scene = m_scenes[filePath];
	assert(scene);

	FbxNode* root = scene->GetRootNode();
	
	AnimationStack* stack = SAIL_NEW AnimationStack();
	m_sceneData[filePath].stack = stack;
	fetchSkeleton(root, filePath, stack);
	fetchAnimations(root, stack, filePath);
	return stack;
}
void FBXLoader::fetchAnimations(FbxNode* node, AnimationStack* stack, const std::string& name) {
	FbxScene* scene = node->GetScene();
	unsigned int numAttributes = node->GetNodeAttributeCount();
	for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; attributeIndex++) {

		FbxNodeAttribute* nodeAttribute = node->GetNodeAttributeByIndex(attributeIndex);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();

		if (attributeType == FbxNodeAttribute::eMesh) {
			FbxMesh* fbxmesh = node->GetMesh();
			
			getAnimations(node, stack, name);
			return;
		}
	}

	unsigned int childCount = (unsigned int)node->GetChildCount();
	for (unsigned int child = 0; child < childCount; child++) {
		fetchAnimations(node->GetChild(child), stack, name);
	}
}
void FBXLoader::getAnimations(FbxNode* node, AnimationStack* stack, const std::string& name) {

	auto& cpMap = m_sceneData[name].cpToVertMap;
	assert(cpMap.size() > 0 && "CPMAP empty :(");
	const FbxScene* scene = node->GetScene();
	std::string nodeName = node->GetName();
	unsigned int numAttributes = node->GetNodeAttributeCount();
	for (unsigned int attributeIndex = 0; attributeIndex < numAttributes; attributeIndex++) {

		
		FbxNodeAttribute* nodeAttribute = node->GetNodeAttributeByIndex(attributeIndex);
		FbxNodeAttribute::EType attributeType = nodeAttribute->GetAttributeType();

		if (attributeType == FbxNodeAttribute::eMesh) {
			FbxMesh* mesh = node->GetMesh();
			unsigned int cpCount = mesh->GetControlPointsCount();
			stack->reSizeConnections(m_sceneData[name].model->getMesh(0)->getNumVertices());

			FbxAMatrix geometryTransform(node->GetGeometricTranslation(FbxNode::eSourcePivot), node->GetGeometricRotation(FbxNode::eSourcePivot), node->GetGeometricScaling(FbxNode::eSourcePivot));

			unsigned int deformerCount = mesh->GetDeformerCount();
			for (unsigned int deformerIndex = 0; deformerIndex < deformerCount; deformerIndex++) {
				FbxSkin* skin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
				if (!skin) {
					#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
						Logger::Error("not a skin at deformer " + std::to_string(deformerIndex) + " in " + nodeName);
					#endif
					continue;
				}
				unsigned int clusterCount = skin->GetClusterCount();

				std::vector<int> limbIndexes;
				limbIndexes.resize(clusterCount);
				for (unsigned int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
					FbxCluster* cluster = skin->GetCluster(clusterIndex);
					limbIndexes[clusterIndex] = getBoneIndex((unsigned int)cluster->GetLink()->GetUniqueID(), name);
					if (limbIndexes[clusterIndex] == -1) {
#if defined(_DEBUG) && defined(SAIL_VERBOSELOGGING)
						Logger::Warning("Could not find limb at clusterIndex: " + std::to_string(clusterIndex));
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

				Animation* defaultAnimation = SAIL_NEW Animation("Default");
				for (unsigned int frameIndex = 0; frameIndex < 3; frameIndex++) {
					Animation::Frame* frame = SAIL_NEW Animation::Frame(stack->boneCount());
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
					Animation* animation = SAIL_NEW Animation(animationName);
					node->GetScene()->SetCurrentAnimationStack(currAnimStack);

					FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
					FbxTime end = takeInfo->mLocalTimeSpan.GetStop();

					float firstFrameTime = 0.0f;

					//TODO: find way to import FPS from file.
					bool firstFrame = true;
					unsigned int offset = 0;
					for (FbxLongLong frameIndex = start.GetFrameCount(fps); frameIndex <= end.GetFrameCount(fps); frameIndex++) {
						Animation::Frame* frame = SAIL_NEW Animation::Frame(stack->boneCount());
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
					Logger::Log("Added: " + animationName);
				}
			}//deformer end
		}
		
	} // attribute end
}
#pragma endregion

#pragma region exportData
Model* FBXLoader::fetchModel(const std::string& filePath, Shader* shader) {
	if (!importScene(filePath, shader)) {
		return nullptr;
	}
	if (m_sceneData.find(filePath) == m_sceneData.end()) {
		return nullptr;
	}
	auto* temp = m_sceneData[filePath].model;
	m_sceneData[filePath].model = nullptr;
	m_sceneData[filePath].hasModel = false;
	return temp;
}
AnimationStack* FBXLoader::fetchAnimationStack(const std::string& filePath, Shader* shader) {
	if (!importScene(filePath, shader)) {
		return nullptr;
	}
	if (m_sceneData.find(filePath) == m_sceneData.end()) {
		return nullptr;
	}
	auto* temp = m_sceneData[filePath].stack;
	m_sceneData[filePath].stack = nullptr;
	m_sceneData[filePath].hasAnimation = false;
	return temp;
}
#pragma endregion



void FBXLoader::addVertex(Mesh::Data& buildData, unsigned int& uniqueVertices, const unsigned long& currentIndex, const Mesh::vec3& position, const Mesh::vec3& normal, const Mesh::vec3& tangent, const Mesh::vec3& bitangent, const Mesh::vec2& uv) {
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
void FBXLoader::fetchSkeleton(FbxNode* node, const std::string& filename, AnimationStack* stack) {


	for (int childIndex = 0; childIndex < node->GetChildCount(); ++childIndex) {
		FbxNode* currNode = node->GetChild(childIndex);
		fetchSkeletonRecursive(currNode, filename, 0, 0, -1, stack);
	}

}
void FBXLoader::fetchSkeletonRecursive(FbxNode* inNode, const std::string& filename, int inDepth, int myIndex, int inParentIndex, AnimationStack* stack) {

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
		fetchSkeletonRecursive(inNode->GetChild(i), filename, inDepth + 1, stack->boneCount(), myIndex, stack);
	}


}
int FBXLoader::getBoneIndex(unsigned int uniqueID, const std::string& name) {
	AnimationStack* stack = m_sceneData[name].stack;
	unsigned int size = stack->boneCount();
	for (unsigned int i = 0; i < size; i++) {
		if (stack->getBone(i).uniqueID == uniqueID) {
			return i;
		}
	}
	return -1;
}

#pragma region DEBUG
//DEBUG
std::string FBXLoader::GetAttributeTypeName(FbxNodeAttribute::EType type) {
	switch (type) {
	case FbxNodeAttribute::eUnknown: return "unidentified";
	case FbxNodeAttribute::eNull: return "null";
	case FbxNodeAttribute::eMarker: return "marker";
	case FbxNodeAttribute::eSkeleton: return "skeleton";
	case FbxNodeAttribute::eMesh: return "mesh";
	case FbxNodeAttribute::eNurbs: return "nurbs";
	case FbxNodeAttribute::ePatch: return "patch";
	case FbxNodeAttribute::eCamera: return "camera";
	case FbxNodeAttribute::eCameraStereo: return "stereo";
	case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
	case FbxNodeAttribute::eLight: return "light";
	case FbxNodeAttribute::eOpticalReference: return "optical reference";
	case FbxNodeAttribute::eOpticalMarker: return "marker";
	case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
	case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
	case FbxNodeAttribute::eBoundary: return "boundary";
	case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
	case FbxNodeAttribute::eShape: return "shape";
	case FbxNodeAttribute::eLODGroup: return "lodgroup";
	case FbxNodeAttribute::eSubDiv: return "subdiv";
	default: return "unknown";
	}
}
std::string FBXLoader::PrintAttribute(FbxNodeAttribute* pAttribute) {
	if (!pAttribute) {
		return "x";
	}

	std::string typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
	std::string attrName = pAttribute->GetName();

	return "("+typeName + " " + attrName+")";
}
void FBXLoader::printNodeTree(FbxNode * node, const std::string& indent) {
	std::string name = node->GetName();
	std::string attributes = "";

	for (int i = 0; i < node->GetNodeAttributeCount(); i++) {
		attributes += ": "+ PrintAttribute(node->GetNodeAttributeByIndex(i));
	}
	
	Logger::Log(indent + name + ":" + attributes);
	for (int i = 0; i < node->GetChildCount(); i++) {
		printNodeTree(node->GetChild(i), indent + "|");
	}
}
void FBXLoader::printAnimationStack(const FbxNode* node) {
}
#pragma endregion