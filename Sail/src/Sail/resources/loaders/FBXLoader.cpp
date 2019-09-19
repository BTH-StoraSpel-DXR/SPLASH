#include "pch.h"
#include "FBXLoader.h"

#include <d3d11.h>
#include "../../utils/Utils.h"
#include "../../graphics/geometry/factory/CubeModel.h"
#include "Sail/Application.h"
#include "Sail/resources/ResourceManager.h"

FbxManager* FBXLoader::s_manager = FbxManager::Create();
FbxIOSettings* FBXLoader::s_ios = FbxIOSettings::Create(s_manager, IOSROOT);

FBXLoader::FBXLoader(const std::string& filepath, Shader* shader)
	: m_filepath(filepath)
	, m_shader(shader)
{
	s_manager->SetIOSettings(s_ios);

	m_model = std::make_unique<Model>();
	m_scene = parseFBX(filepath);
	
	// Triangulate all meshes in the scene
	FbxGeometryConverter geoConverter(s_manager);
	geoConverter.Triangulate(m_scene, true);

	if (m_scene != nullptr) {

		loadNode(m_scene->GetRootNode());
		/*m_model->getMaterial()->setKa(m_matSettings.ka);
		m_model->getMaterial()->setKd(m_matSettings.kd);
		m_model->getMaterial()->setKs(m_matSettings.ks);
		m_model->getMaterial()->setShininess(m_matSettings.shininess);
		if (m_matDiffuseTex != "")
			m_model->getMaterial()->setDiffuseTexture(m_matDiffuseTex);
		if (m_matNormalTex != "")
			m_model->getMaterial()->setDiffuseTexture(m_matNormalTex);
		if (m_matSpecularTex != "")
			m_model->getMaterial()->setDiffuseTexture(m_matSpecularTex);*/

	} else {
		Logger::Warning("Failed to load fbx file '" + filepath + "', using default cube.");
		glm::vec3 halfSizes = glm::vec3(0.5, 0.5, 0.5);
		m_model = ModelFactory::CubeModel::Create(halfSizes, shader);
	}
}

FBXLoader::~FBXLoader() {
	if (m_scene)
		m_scene->Destroy();
}

std::unique_ptr<Model>& FBXLoader::getModel() {
	return m_model;
}

FbxScene* FBXLoader::parseFBX(const std::string& filename) {
	FbxImporter* importer = FbxImporter::Create(s_manager, "");

	if (!importer->Initialize(filename.c_str(), -1, s_manager->GetIOSettings()))
		return nullptr;

	FbxScene* scene = FbxScene::Create(s_manager, "scene");
	importer->Import(scene);

	importer->Destroy();

	return scene;
}

void FBXLoader::loadNode(FbxNode* pNode) {
	// The number of attributes for the current node (mesh for example)
	int numAttributes = pNode->GetNodeAttributeCount();

	for (int j = 0; j < numAttributes; j++) {
		FbxNodeAttribute *nodeAttributeFbx = pNode->GetNodeAttributeByIndex(j);
		FbxNodeAttribute::EType attributeType = nodeAttributeFbx->GetAttributeType();

		FbxMesh* mesh;
		if (attributeType == FbxNodeAttribute::eMesh) {
			// Gets the mesh (only one mesh per model is supported)
			mesh = (FbxMesh*)nodeAttributeFbx;

			Mesh::Data meshData;
			getGeometry(mesh, meshData);
			std::unique_ptr<Mesh> mesh = std::unique_ptr<Mesh>(Mesh::Create(meshData, m_shader));
			getMaterial(pNode, mesh->getMaterial());
			m_model->addMesh(std::move(mesh));

		}

		if (attributeType == FbxNodeAttribute::eSkeleton) {
			// To load skeleton data for future versions of the engine
		}
	}

	// Recursively load the child nodes
	int numChildren = pNode->GetChildCount();
	for (int i = 0; i < numChildren; i++)
		this->loadNode(pNode->GetChild(i));

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

void FBXLoader::getGeometry(FbxMesh* mesh, Mesh::Data& buildData) {

	// Number of polygon vertices 
	buildData.numVertices = mesh->GetPolygonVertexCount();
	int* indices = mesh->GetPolygonVertices();

	if (int(buildData.numVertices / 3) != mesh->GetPolygonCount()) {
		Logger::Error("The mesh in '" + m_filepath + "' has to be triangulated.");
		return;
	}

	buildData.positions = SAIL_NEW Mesh::vec3[buildData.numVertices];
	buildData.normals = SAIL_NEW Mesh::vec3[buildData.numVertices];
	buildData.texCoords = SAIL_NEW Mesh::vec2[buildData.numVertices];
	buildData.tangents = SAIL_NEW Mesh::vec3[buildData.numVertices];
	buildData.bitangents = SAIL_NEW Mesh::vec3[buildData.numVertices];

	bool norms = true, uvs = true, tangs = true, bitangs = true;

	int vertexIndex = 0;
	FbxVector4* cp = mesh->GetControlPoints();
	if (cp == nullptr) {
		Logger::Error("Couldn't find any vertices in the mesh in the file " + m_filepath);
		return;
	}
	for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++) {
		int numVertices = mesh->GetPolygonSize(polyIndex);

		for (int vertIndex = 0; vertIndex < numVertices; vertIndex += 3) {

			/*
			--	Positions
			*/
			buildData.positions[vertexIndex].vec.x = -(float)cp[indices[vertexIndex]][0];
			buildData.positions[vertexIndex].vec.y = (float)cp[indices[vertexIndex]][1];
			buildData.positions[vertexIndex].vec.z = (float)cp[indices[vertexIndex]][2];

			buildData.positions[vertexIndex + 1].vec.x = -(float)cp[indices[vertexIndex + 2]][0];
			buildData.positions[vertexIndex + 1].vec.y = (float)cp[indices[vertexIndex + 2]][1];
			buildData.positions[vertexIndex + 1].vec.z = (float)cp[indices[vertexIndex + 2]][2];

			buildData.positions[vertexIndex + 2].vec.x = -(float)cp[indices[vertexIndex + 1]][0];
			buildData.positions[vertexIndex + 2].vec.y = (float)cp[indices[vertexIndex + 1]][1];
			buildData.positions[vertexIndex + 2].vec.z = (float)cp[indices[vertexIndex + 1]][2];


			/*
			--	Normals
			*/
			FbxLayerElementNormal* leNormal = mesh->GetLayer(0)->GetNormals();
			if (leNormal == nullptr && norms) {
				Logger::Warning("Couldn't find any normals in the mesh in the file " + m_filepath);
				norms = false;
			} else if (norms) {
				if (leNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
					int normIndex = 0;

					if (leNormal->GetReferenceMode() == FbxLayerElement::eDirect)
						normIndex = vertexIndex;


					if (leNormal->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
						normIndex = leNormal->GetIndexArray().GetAt(vertexIndex);


					FbxVector4 norm = leNormal->GetDirectArray().GetAt(normIndex);
					buildData.normals[vertexIndex].vec.x = -(float)norm[0];
					buildData.normals[vertexIndex].vec.y = (float)norm[1];
					buildData.normals[vertexIndex].vec.z = (float)norm[2];

					norm = leNormal->GetDirectArray().GetAt(normIndex + 2);
					buildData.normals[vertexIndex + 1].vec.x = -(float)norm[0];
					buildData.normals[vertexIndex + 1].vec.y = (float)norm[1];
					buildData.normals[vertexIndex + 1].vec.z = (float)norm[2];

					norm = leNormal->GetDirectArray().GetAt(normIndex + 1);
					buildData.normals[vertexIndex + 2].vec.x = -(float)norm[0];
					buildData.normals[vertexIndex + 2].vec.y = (float)norm[1];
					buildData.normals[vertexIndex + 2].vec.z = (float)norm[2];
				}
			}

			/*
			--	Tangents
			*/
			FbxGeometryElementTangent *geTang = mesh->GetElementTangent(0);
			if (geTang == nullptr && tangs) {
				Logger::Warning("Couldn't find any tangents in the mesh in the file " + m_filepath);
				tangs = false;
			} else if (tangs) {
				if (geTang->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
					int tangIndex = 0;

					if (geTang->GetReferenceMode() == FbxLayerElement::eDirect)
						tangIndex = vertexIndex;


					if (geTang->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
						tangIndex = geTang->GetIndexArray().GetAt(vertexIndex);

					FbxVector4 tangent = geTang->GetDirectArray().GetAt(tangIndex);
					buildData.tangents[vertexIndex].vec.x = (float)tangent[0];
					buildData.tangents[vertexIndex].vec.y = (float)tangent[1];
					buildData.tangents[vertexIndex].vec.z = (float)tangent[2];

					tangent = geTang->GetDirectArray().GetAt(tangIndex + 2);
					buildData.tangents[vertexIndex + 1].vec.x = (float)tangent[0];
					buildData.tangents[vertexIndex + 1].vec.y = (float)tangent[1];
					buildData.tangents[vertexIndex + 1].vec.z = (float)tangent[2];

					tangent = geTang->GetDirectArray().GetAt(tangIndex + 1);
					buildData.tangents[vertexIndex + 2].vec.x = (float)tangent[0];
					buildData.tangents[vertexIndex + 2].vec.y = (float)tangent[1];
					buildData.tangents[vertexIndex + 2].vec.z = (float)tangent[2];
				}
			}

			/*
			--	Binormals
			*/
			FbxGeometryElementBinormal *geBN = mesh->GetElementBinormal(0);
			if (geBN == nullptr && bitangs) {
				Logger::Warning("Couldn't find any binormals in the mesh in the file " + m_filepath);
				bitangs = false;
			} else if (bitangs) {
				if (geBN->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
					int biNormIndex = 0;

					if (geBN->GetReferenceMode() == FbxLayerElement::eDirect)
						biNormIndex = vertexIndex;


					if (geBN->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
						biNormIndex = geBN->GetIndexArray().GetAt(vertexIndex);

					FbxVector4 biNorm = geBN->GetDirectArray().GetAt(biNormIndex);
					buildData.bitangents[vertexIndex].vec.x = (float)biNorm[0];
					buildData.bitangents[vertexIndex].vec.y = (float)biNorm[1];
					buildData.bitangents[vertexIndex].vec.z = (float)biNorm[2];

					biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 2);
					buildData.bitangents[vertexIndex + 1].vec.x = (float)biNorm[0];
					buildData.bitangents[vertexIndex + 1].vec.y = (float)biNorm[1];
					buildData.bitangents[vertexIndex + 1].vec.z = (float)biNorm[2];

					biNorm = geBN->GetDirectArray().GetAt(biNormIndex + 1);
					buildData.bitangents[vertexIndex + 2].vec.x = (float)biNorm[0];
					buildData.bitangents[vertexIndex + 2].vec.y = (float)biNorm[1];
					buildData.bitangents[vertexIndex + 2].vec.z = (float)biNorm[2];
				}
			}

			/*
			--	UV Coords
			*/
			FbxGeometryElementUV* geUV = mesh->GetElementUV(0);
			if (geUV == nullptr && uvs) {
				Logger::Warning("Couldn't find any texture coordinates in the mesh in the file " + m_filepath);
				uvs = false;
			} else if (uvs) {
				FbxVector2 texCoord;
				int cpIndex;

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex);
				buildData.texCoords[vertexIndex].vec.x = static_cast<float>(texCoord[0]);
				buildData.texCoords[vertexIndex].vec.y = -static_cast<float>(texCoord[1]);

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 2);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 2);
				buildData.texCoords[vertexIndex + 1].vec.x = static_cast<float>(texCoord[0]);
				buildData.texCoords[vertexIndex + 1].vec.y = -static_cast<float>(texCoord[1]);

				cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex + 1);
				texCoord = getTexCoord(cpIndex, geUV, mesh, polyIndex, vertIndex + 1);
				buildData.texCoords[vertexIndex + 2].vec.x = static_cast<float>(texCoord[0]);
				buildData.texCoords[vertexIndex + 2].vec.y = -static_cast<float>(texCoord[1]);
			}

			vertexIndex += 3;
		}

	}

}

void FBXLoader::getMaterial(FbxNode* pNode, Material* material) {

	// Gets the model's phong constants
	if (pNode->GetSrcObjectCount<FbxSurfacePhong>() > 0) {

		auto phong = pNode->GetSrcObject<FbxSurfacePhong>();
		auto& resman = *Application::getInstance()->getResourceManager();

		FbxFileTexture* diffTex = phong->Diffuse.GetSrcObject<FbxFileTexture>();
		if (diffTex) {
			std::string filename = diffTex->GetRelativeFileName();
			resman.loadTexture(filename);
			material->setDiffuseTexture(filename);
		}
		FbxFileTexture* specTex = phong->Specular.GetSrcObject<FbxFileTexture>();
		if (specTex) {
			std::string filename = specTex->GetRelativeFileName();
			resman.loadTexture(filename);
			material->setSpecularTexture(specTex->GetRelativeFileName());
		}
		FbxFileTexture* normTex = phong->NormalMap.GetSrcObject<FbxFileTexture>();
		if (normTex) {
			std::string filename = normTex->GetRelativeFileName();
			resman.loadTexture(filename);
			material->setNormalTexture(normTex->GetRelativeFileName());
		}

		material->setKa(static_cast<float>(phong->AmbientFactor.Get()));
		material->setKs(static_cast<float>(phong->SpecularFactor.Get()));
		material->setKd (static_cast<float>(phong->DiffuseFactor.Get()));
		material->setShininess(static_cast<float>(phong->Shininess.Get()));

	}

}
