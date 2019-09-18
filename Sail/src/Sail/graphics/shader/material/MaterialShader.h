#pragma once

//#include <d3d11.h>
//#include <glm/glm.hpp>
#include <glm/vec4.hpp>
//#include "Sail/api/shader/ShaderPipeline.h"
//#include "../../Lights.h"
#include "Sail/graphics/shader/Shader.h"
//#include "../../geometry/Material.h"

class MaterialShader : public Shader {
public:
	MaterialShader();
	~MaterialShader();

	virtual void bind() override;
	virtual void setClippingPlane(const glm::vec4& clippingPlane) override;

	//void updateLights(const Lights& lights);

	// TODO: remove this
	/*struct Vertex {
		glm::vec3 position;
		glm::vec2 texCoords;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};*/

private:
	/*void updateModelDataBuffer(const Material& material, const glm::mat4& w, const glm::mat4& vp) const;
	void updateWorldDataBuffer() const;*/

private:
	// Transform constant buffer structure
	//struct ModelDataBuffer {
	//	glm::mat4 mWorld;
	//	glm::mat4 mVP;
	//	glm::vec4 modelColor;
	//	float ka;
	//	float kd;
	//	float ks;
	//	float shininess;
	//	int textureFlags[3];
	//	int padding;
	//};
	//struct WorldDataBuffer {
	//	glm::vec4 clippingPlane;
	//	glm::vec3 cameraPos;
	//	float padding;
	//};
	//struct DirLightBuffer {
	//	Lights::DirectionalLight dirLight;
	//};
	//struct PointLightsBuffer {
	//	Lights::PointLightStruct pLights[8]; // Max number of lights as set in shader
	//	float padding1, padding2, padding3, padding4;
	//};
	/*DirLightBuffer m_dirLightData;
	PointLightsBuffer m_pointLightsData;*/
	/*glm::mat4 m_vpMatrix;*/
	glm::vec4 m_clippingPlane;
	//glm::vec3 m_cameraPos;
	bool m_clippingPlaneHasChanged;
	//bool m_cameraPosHasChanged;

	// Components

	/*std::unique_ptr<ShaderComponent::ConstantBuffer> m_modelDataBuffer;
	std::unique_ptr<ShaderComponent::ConstantBuffer> m_worldDataBuffer;
	std::unique_ptr<ShaderComponent::ConstantBuffer> m_lightsBuffer;*/
	//std::unique_ptr<ShaderComponent::Sampler> m_sampler;

};
