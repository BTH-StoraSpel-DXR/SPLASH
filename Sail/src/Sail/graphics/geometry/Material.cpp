#include "pch.h"
#include "Material.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/Application.h"
#include "Sail/resources/ResourceManager.h"

Material::Material(Shader* shader)
	: m_numTextures(3)
	, m_shader(shader)
	, m_textures {nullptr}
{
	m_phongSettings.ka = 1.f;
	m_phongSettings.kd = 1.f;
	m_phongSettings.ks = 1.f;
	m_phongSettings.shininess = 10.f;
	m_phongSettings.modelColor = glm::vec4(1.0f);
	m_phongSettings.hasDiffuseTexture = 0;
	m_phongSettings.hasNormalTexture = 0;
	m_phongSettings.hasSpecularTexture = 0;

}
Material::~Material() { }

void Material::bind(void* cmdList) {
	ShaderPipeline* pipeline = m_shader->getPipeline();
	pipeline->trySetCBufferVar("sys_material", (void*)&getPhongSettings(), sizeof(PhongSettings));

	if (m_phongSettings.hasDiffuseTexture)
		pipeline->setTexture2D("sys_texDiffuse", m_textures[0], cmdList);
	if (m_phongSettings.hasNormalTexture)
		pipeline->setTexture2D("sys_texNormal", m_textures[1], cmdList);
	if (m_phongSettings.hasSpecularTexture)
		pipeline->setTexture2D("sys_texSpecular", m_textures[2], cmdList);
	//m_shader->bind();
}

void Material::setKa(float ka) {
	m_phongSettings.ka = ka;
}
void Material::setKd(float kd) {
	m_phongSettings.kd = kd;
}
void Material::setKs(float ks) {
	m_phongSettings.ks = ks;
}
void Material::setShininess(float shininess) {
	m_phongSettings.shininess = shininess;
}
void Material::setColor(const glm::vec4& color) {
	m_phongSettings.modelColor = color;
}


void Material::setDiffuseTexture(const std::string& filename) {
	getAndInsertTexture(filename, 0);
	m_phongSettings.hasDiffuseTexture = 1;
}
void Material::setDiffuseTextureFromHandle(Texture* srv) {
	m_textures[0] = srv;
	m_phongSettings.hasDiffuseTexture = 1;
}


void Material::setNormalTexture(const std::string& filename) {
	getAndInsertTexture(filename, 1);
	m_phongSettings.hasNormalTexture = 1;
}
void Material::setNormalTextureFromHandle(Texture* srv) {
	m_textures[1] = srv;
	m_phongSettings.hasNormalTexture = 1;
}


void Material::setSpecularTexture(const std::string& filename) {
	getAndInsertTexture(filename, 2);
	m_phongSettings.hasSpecularTexture = 1;
}
void Material::setSpecularTextureFromHandle(Texture* srv) {
	m_textures[2] = srv;
	m_phongSettings.hasSpecularTexture = 1;
}


//void Material::setTextures(ID3D11ShaderResourceView** srvs, UINT numTextures) {
//	m_numTextures = numTextures;
//	m_customSRVs = srvs;
//}


void Material::getAndInsertTexture(const std::string& filename, int arraySlot) {
	Texture* t = &Application::getInstance()->getResourceManager()->getTexture(filename);
	m_textures[arraySlot] = t;
}

//ID3D11ShaderResourceView* const* Material::getTextures(UINT& numTextures) {
//	numTextures = m_numTextures;
//	if (m_customSRVs)
//		return m_customSRVs;
//	else
//		return m_srvs;
//}

const Material::PhongSettings& Material::getPhongSettings() const {
	return m_phongSettings;
}

Shader* Material::getShader() const {
	return m_shader;
}

// TODO: remove
//float Material::getKa() const {
//	return m_phongSettings.ka;
//}
//float Material::getKd() const {
//	return m_phongSettings.kd;
//}
//float Material::getKs() const {
//	return m_phongSettings.ks;
//}
//float Material::getShininess() const {
//	return m_phongSettings.shininess;
//}
//const glm::vec4& Material::getColor() const {
//	return m_phongSettings.modelColor;
//}
//const bool* Material::getTextureFlags() const {
//	return m_phongSettings.textureFlags;
//}