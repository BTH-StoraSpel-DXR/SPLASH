#pragma once

#include "Component.h"
#include "../../graphics/light/PointLight.h"

class LightComponent : public Component<LightComponent> {
public:
	//create point light
	LightComponent(PointLight pl) : defaultColor(pl.getColor()) {
		m_pointLight.setRadius(pl.getRadius());
		m_pointLight.setColor(pl.getColor());
		m_pointLight.setPosition(pl.getPosition());
		m_pointLight.setIndex(pl.getIndex());
	}
	~LightComponent() {}

	PointLight& getPointLight() {
		return m_pointLight;
	}
	
	const glm::vec3 getInterpolatedPosition(float alpha) const {
		return (alpha * currentPos) + ((1.0f - alpha) * prevPos);
	}

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this);
	}
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


		float reachRadius = m_pointLight.getRadius();
		bool changed = false;

		if (ImGui::DragFloat("##radius", &reachRadius, 0.01f)) {
			changed = true;
		} 
		ImGui::NextColumn();
		ImGui::Text(std::string("Radius").c_str()); ImGui::NextColumn();
		ImGui::Columns(1);
	}
#endif

public:
	glm::vec3 defaultColor = { 1.0f, 1.0f, 1.0f };
	glm::vec3 prevPos = { 0.f, 0.f, 0.f };
	glm::vec3 currentPos = { 0.f, 0.f, 0.f };
private:
	PointLight m_pointLight;

};