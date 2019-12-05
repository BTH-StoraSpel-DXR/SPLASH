#pragma once
#include "Component.h"
#include "../../graphics/light/PointLight.h"

class HazardLightSystem;


class SpotlightComponent final : public Component<SpotlightComponent> {
public:
	SpotlightComponent(){}
	~SpotlightComponent() {}

	bool isOn;
	float alarmTimer = 0.f;
	int roomID;
	SpotLight light; // Describes the light source when not rotated or moved

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this);
	}
	void imguiRender(Entity** selected) {
		ImGui::Columns(2);

		if (ImGui::Button("Toggle ON/OFF")) {
			isOn = !isOn;
		}
		ImGui::NextColumn();
		ImGui::Text(std::string(isOn ? "On" : "OFF").c_str()); ImGui::NextColumn();

		glm::vec col = light.getColor();
		if (ImGui::DragFloat3("##COLOR", &col.x, 0.1f)) {
			light.setColor(col);
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("Color").c_str()); ImGui::NextColumn();

		glm::vec pos = light.getPosition();
		if (ImGui::DragFloat3("##POSITION", &pos.x, 0.1f)) {
			light.setPosition(pos);
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("Relative Position").c_str()); ImGui::NextColumn();

		glm::vec rel_dir = light.getDirection();
		if (ImGui::DragFloat3("##DIRECTION", &rel_dir.x, 0.1f)) {
			light.setDirection(rel_dir);
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("Relative Direction").c_str()); ImGui::NextColumn();

		glm::vec abs_dir = m_lightEntityRotated.getDirection();
		if (ImGui::DragFloat3("##ABSDIRECTION", &abs_dir.x, 0.1f)) {
			light.setDirection(abs_dir);
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("Absolute Direction").c_str()); ImGui::NextColumn();

		float angle = light.getAngle();
		if (ImGui::DragFloat("##Angle", &angle, 0.01f, 0.01, 1.0f)) {
			light.setAngle(angle);
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("Angle").c_str()); ImGui::NextColumn();

		float constant = light.getAttenuation().constant;
		float linear = light.getAttenuation().linear;
		float quadratic = light.getAttenuation().quadratic;
		bool changed = false;

		if (ImGui::DragFloat("##constant", &constant, 0.01f)) {
			changed = true;
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("constant").c_str()); ImGui::NextColumn();

		if (ImGui::DragFloat("##linear", &linear, 0.01f)) {
			changed = true;
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("linear").c_str()); ImGui::NextColumn();

		if (ImGui::DragFloat("##quadratic", &quadratic, 0.01f)) {
			changed = true;
		}
		ImGui::NextColumn();
		ImGui::Text(std::string("quadratic").c_str()); ImGui::NextColumn();
		if (changed) {
			light.setAttenuation(constant, linear, quadratic);
		}
		ImGui::Columns(1);
	}
#endif
private:
	SpotLight m_lightEntityRotated; //This one will be updated with entity transformations and submited to the renderer

	friend class HazardLightSystem;
	
};
