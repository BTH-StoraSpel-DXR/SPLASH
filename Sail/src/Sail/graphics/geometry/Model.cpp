#include "pch.h"
#include "Model.h"

//#include "../shader/basic/SimpleColorShader.h"
#include "Material.h"

Model::Model(Mesh::Data& buildData, Shader* shader) {

	m_meshes.push_back(std::unique_ptr<Mesh>(Mesh::Create(buildData, shader)));

	// TODO: reuse materials (?)
	//m_material = std::make_shared<Material>(shaderSet);
}

Model::Model() {

}

//Model::Model(const std::string& path, ShaderSet* shaderSet) {
//	// TODO: reuse mesh if it has already been loaded
//	// TODO: load mesh from FBX model specified by path
//	//m_mesh = std::make_shared<Mesh>(buildData, shaderSet);
//
//	// TODO: reuse materials (?)
//	m_material = std::make_shared<Material>(shaderSet);
//}

//Model::Model(std::vector<Mesh::Data>& data, ShaderSet* shaderSet) {
//
//	for (auto& meshData : data) {
//		m_meshes.push_back(std::make_shared<Mesh>(meshData, shaderSet));
//	}
//
//	//m_material = std::make_shared<Material>(shaderSet);
//
//}

//Model::Model(ShaderSet* shaderSet)
//	: m_aabb(glm::vec3(0.f), glm::vec3(.2f, .2f, .2f))
//	, m_vertexBuffer(nullptr)
//	, m_indexBuffer(nullptr)
//	, m_shader(shaderSet)
//{
//	m_material = SAIL_NEW Material();
//	m_transform = SAIL_NEW Transform();
//	m_transformChanged = false;
//}
Model::~Model() {
}

Mesh* Model::addMesh(std::unique_ptr<Mesh> mesh) {
	m_meshes.push_back(std::move(mesh));
	return m_meshes.back().get();
}

//void Model::setBuildData(Data& buildData) {
//	m_data = buildData;
//}
//const Model::Data& Model::getBuildData() const {
//	return m_data;
//}

//void Model::buildBufferForShader(ShaderSet* shader) {
//
//	shader->createBufferFromModelData(&m_vertexBuffer, &m_indexBuffer, &m_instanceBuffer, &m_data);
//	calculateAABB();
//
//}

//void Model::draw(const Renderer& renderer) {
//
//	//m_material->bind();
//	for (auto& mesh : m_meshes)
//		mesh->draw(renderer);
//
//}

Mesh* Model::getMesh(unsigned int index) {
	assert(m_meshes.size() > index);
	return m_meshes[index].get();
}

unsigned int Model::getNumberOfMeshes() const {
	return m_meshes.size();
}

//ShaderSet* Model::getShader() const {
//	//return m_material->getShader();
//	return nullptr;
//}
//
//Material* Model::getMaterial() {
//	//return m_material.get();
//	return nullptr;
//}

//const AABB& Model::getAABB() const {
//	return m_aabb;
//}
//void Model::updateAABB() {
//	m_aabb.updateTransform(m_transform->getMatrix());
//}
//
//void Model::calculateAABB() {
//
//	glm::vec3 minCorner(FLT_MAX, FLT_MAX, FLT_MAX );
//	glm::vec3 maxCorner(-FLT_MIN, -FLT_MIN, -FLT_MIN);
//
//	for (UINT i = 0; i < m_data.numVertices; i++) {
//		glm::vec3& p = m_data.positions[i];
//
//		if (p.x < minCorner.x) minCorner.x = p.x;
//		if (p.y < minCorner.y) minCorner.y = p.y;
//		if (p.z < minCorner.z) minCorner.z = p.z;
//
//		if (p.x > maxCorner.x) maxCorner.x = p.x;
//		if (p.y > maxCorner.y) maxCorner.y = p.y;
//		if (p.z > maxCorner.z) maxCorner.z = p.z;
//
//	}
//
//	m_aabb.setMinPos(minCorner);
//	m_aabb.setMaxPos(maxCorner);
//
//}
