#pragma once

#include "Component.h"
#include "../../graphics/light/PointLight.h"

class LightComponent : public Component<LightComponent> {
public:
	//create point light
	LightComponent(PointLight pl) : defaultColor(pl.getColor()) {
		m_pointLight.setAttenuation(pl.getAttenuation().constant,pl.getAttenuation().linear,pl.getAttenuation().quadratic);
		m_pointLight.setColor(pl.getColor());
		m_pointLight.setPosition(pl.getPosition());
		m_pointLight.setIndex(pl.getIndex());
	}
	~LightComponent() {}

	PointLight& getPointLight() {
		return m_pointLight;
	}
#ifdef DEVELOPMENT
	void imguiRender(Entity** selected) {
		ImGui::Columns(2);
		glm::vec col = m_pointLight.getColor();
		if (ImGui::DragFloat3("##COLOR", &col.x, 0.1f)) {
			m_pointLight.setColor(col);
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("Color").c_str()); ImGui::NextColumn();


		glm::vec pos = m_pointLight.getPosition();
		if (ImGui::DragFloat3("##POSITION", &pos.x, 0.1f)) {
			m_pointLight.setPosition(pos);
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("Position").c_str()); ImGui::NextColumn();


		float constant = m_pointLight.getAttenuation().constant;
		float linear = m_pointLight.getAttenuation().linear;
		float quadratic = m_pointLight.getAttenuation().quadratic;
		bool changed = false;

		if (ImGui::DragFloat("##constant", &constant, 0.01f)) {
			changed = true;
		} 
		ImGui::NextColumn();
		ImGui::Text(std::string("constant").c_str()); ImGui::NextColumn();

		if(ImGui::DragFloat("##linear", &linear, 0.01f)) {
			changed = true;
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("linear").c_str()); ImGui::NextColumn();

		if(ImGui::DragFloat("##quadratic", &quadratic, 0.01f)) {
			changed = true;
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("quadratic").c_str()); ImGui::NextColumn();
		if (changed) {
			m_pointLight.setAttenuation(constant, linear, quadratic);
		}
		ImGui::Columns(1);




	}
#endif


	glm::vec3 defaultColor = { 1.0f, 1.0f, 1.0f };
private:
	PointLight m_pointLight;

};