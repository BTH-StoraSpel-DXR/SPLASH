#include "pch.h"
#include "PBRMaterial.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/Application.h"

PBRMaterial::PBRMaterial(Shader* shader)
	: m_numTextures(3)
	, m_shader(shader)
	, m_textures{ nullptr }
{
	m_pbrSettings.metalnessScale = 1.f;
	m_pbrSettings.roughnessScale = 1.f;
	m_pbrSettings.aoScale = 1.f;

	m_pbrSettings.hasAlbedoTexture = 0;
	m_pbrSettings.hasNormalTexture = 0;
	m_pbrSettings.hasMetalnessRoughnessAOTexture = 0;

	m_pbrSettings.modelColor = glm::vec4(1.0f);
}
PBRMaterial::~PBRMaterial() {}

/*Not safe for multithreaded commandlist recording(d3d12)*/
void PBRMaterial::bind(void* cmdList) {
	ShaderPipeline* pipeline = m_shader->getPipeline();
	pipeline->trySetCBufferVar("sys_material_pbr", (void*)&getPBRSettings(), sizeof(PBRSettings));

	// TODO: check if this causes a problem in DX12
	// when a normal or specular texture is bound but not a diffuse one, the order will probably be wrong in dx12 shaders
	if (m_pbrSettings.hasAlbedoTexture) {
		pipeline->setTexture2D("sys_texAlbedo", m_textures[0], cmdList);
	}
	if (m_pbrSettings.hasNormalTexture) {
		pipeline->setTexture2D("sys_texNormal", m_textures[1], cmdList);
	}
	if (m_pbrSettings.hasMetalnessRoughnessAOTexture) {
		pipeline->setTexture2D("sys_texMetalnessRoughnessAO", m_textures[2], cmdList);
	}
}

void PBRMaterial::setMetalnessScale(float metalness) {
	m_pbrSettings.metalnessScale = metalness;
}

void PBRMaterial::setRoughnessScale(float roughness) {
	m_pbrSettings.roughnessScale = roughness;
}

void PBRMaterial::setAOScale(float ao) {
	m_pbrSettings.aoScale = ao;
}

void PBRMaterial::setColor(const glm::vec4& color) {
	m_pbrSettings.modelColor = color;
}

void PBRMaterial::setAlbedoTexture(const std::string& filename) {
	getAndInsertTexture(filename, 0);
	m_pbrSettings.hasAlbedoTexture = 1;
}

void PBRMaterial::setNormalTexture(const std::string& filename) {
	getAndInsertTexture(filename, 1);
	m_pbrSettings.hasNormalTexture = 1;
}

void PBRMaterial::setMetalnessRoughnessAOTexture(const std::string& filename) {
	getAndInsertTexture(filename, 2);
	m_pbrSettings.hasMetalnessRoughnessAOTexture = 1;
}

void PBRMaterial::manuallySetTexture(Texture* tex, unsigned int arraySlot) {
	m_textures[arraySlot] = tex;
	switch (arraySlot) {
	case 0:
		m_pbrSettings.hasAlbedoTexture = 1;
		break;
	case 1:
		m_pbrSettings.hasNormalTexture = 1;
		break;
	case 2:
		m_pbrSettings.hasMetalnessRoughnessAOTexture = 1;
		break;
	default:
		SAIL_LOG_ERROR("Defuq @ manuallySetTexture");
		break;
	}
}

void PBRMaterial::getAndInsertTexture(const std::string& filename, int arraySlot) {
	Texture* t = &Application::getInstance()->getResourceManager().getTexture(filename);
	m_textures[arraySlot] = t;
}

Texture* PBRMaterial::getTexture(unsigned int id) const {
	return m_textures[id];
}

const PBRMaterial::PBRSettings& PBRMaterial::getPBRSettings() const {
	return m_pbrSettings;
}

Shader* PBRMaterial::getShader() const {
	return m_shader;
}
