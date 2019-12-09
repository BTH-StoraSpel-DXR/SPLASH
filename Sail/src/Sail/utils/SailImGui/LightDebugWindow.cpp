#include "pch.h"

#include "LightDebugWindow.h"
#include "imgui.h"
#include "Sail/graphics/light/LightSetup.h"

LightDebugWindow::LightDebugWindow(bool showWindow)
	: m_lightSetup(nullptr)
	, m_manualOverride(false)
{}

LightDebugWindow::~LightDebugWindow() {

}

void LightDebugWindow::setLightSetup(LightSetup* lights) {
	m_lightSetup = lights;
}

bool LightDebugWindow::isManualOverrideOn() {
	return m_manualOverride;
}

void LightDebugWindow::setManualOverride(bool enable) {
	m_manualOverride = enable;
}

void LightDebugWindow::renderWindow() {
	ImGui::Begin("Light debug");
	ImGui::Checkbox("Manual override", &m_manualOverride);
	if (m_lightSetup) {
		unsigned int i = 0;
		for (auto& pl : m_lightSetup->getPLs()) {
			ImGui::PushID(i);
			std::string label("Point light ");
			label += std::to_string(i);
			if (ImGui::CollapsingHeader(label.c_str())) {

				glm::vec3 color = pl.getColor(); // = 1.0f
				glm::vec3 position = pl.getPosition(); // (12.f, 4.f, 0.f);
				float reachRadius = pl.getRadius(); // 10.f;

				ImGui::SliderFloat3("Color##", &color[0], 0.f, 1.0f);
				ImGui::SliderFloat3("Position##", &position[0], -15.f, 15.0f);
				ImGui::SliderFloat("Radius##", &reachRadius, 0.f, 50.f);

				pl.setRadius(reachRadius);
				pl.setColor(color);
				pl.setPosition(position);

			}
			i++;
			ImGui::PopID();
		}
		i = 0;
		for (auto& sl : m_lightSetup->getSLs()) {
			ImGui::PushID(i);
			std::string label("Spot light ");
			label += std::to_string(i);
			if (ImGui::CollapsingHeader(label.c_str())) {

				glm::vec3 color = sl.getColor();
				glm::vec3 position = sl.getPosition();
				float reachRadius = sl.getRadius(); // 10.f;
				float angle = sl.getAngle();
				glm::vec3 direction = sl.getDirection();

				ImGui::SliderFloat3("Color##", &color[0], 0.f, 1.0f);
				ImGui::SliderFloat3("Position##", &position[0], -15.f, 15.0f);
				ImGui::SliderFloat3("Direction##", &direction[0], -1.f, 1.f);
				ImGui::SliderFloat("Angle##", &angle, 0.f, glm::two_pi<float>());
				ImGui::SliderFloat("Radius##", &reachRadius, 0.f, 50.f);

				sl.setRadius(reachRadius);
				sl.setColor(color);
				sl.setPosition(position);
				sl.setAngle(angle);
				sl.setDirection(direction);

			}
			i++;
			ImGui::PopID();
		}
	}
	ImGui::End();
}
