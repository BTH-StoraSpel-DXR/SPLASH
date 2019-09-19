#include "pch.h"
#include "Model.h"
#include "Material.h"

Model::Model(Mesh::Data& buildData, Shader* shader) {

	m_meshes.push_back(std::unique_ptr<Mesh>(Mesh::Create(buildData, shader)));

	// TODO: reuse materials (?)
	//m_material = std::make_shared<Material>(shaderSet);
}

Model::Model() {

}

Model::~Model() {
}

Mesh* Model::addMesh(std::unique_ptr<Mesh> mesh) {
	m_meshes.push_back(std::move(mesh));
	return m_meshes.back().get();
}

Mesh* Model::getMesh(unsigned int index) {
	assert(m_meshes.size() > index);
	return m_meshes[index].get();
}

unsigned int Model::getNumberOfMeshes() const {
	return m_meshes.size();
}