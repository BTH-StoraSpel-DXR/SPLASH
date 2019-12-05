#include "pch.h"
#include "PowerUpComponent.h"

PowerUpComponent::PowerUpComponent() {

	powerUps.push_back({ 0, 60, "RUNSPEED" });
	powerUps.push_back({ 0, 60, "STAMINA" });
	powerUps.push_back({ 0, 60, "SHOWER" });
	powerUps.push_back({ 0, 60, "POWERWASH" });
}

PowerUpComponent::~PowerUpComponent() {
}
#ifdef DEVELOPMENT
void PowerUpComponent::imguiRender(Entity** selected) {

	if(ImGui::CollapsingHeader("PickUps##adders")) {
		static float value = 30.0f;
		ImGui::SliderFloat("value to add", &value, 0.0f, 60.0f);
		for (auto& pow : powerUps) {
			if (ImGui::Button(pow.name.c_str())) {
				pow.addTime(value);
			}
		}
	}


	ImGui::Columns(2, nullptr, false);
	for (auto& pow : powerUps) {
		ImGui::Text(pow.name.c_str()); 
		ImGui::NextColumn();
		ImGui::ProgressBar(pow.time / pow.maxTime, ImVec2(-1, 0), std::string(std::to_string((int)pow.time) + " / " + std::to_string((int)pow.maxTime)).c_str());
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
}
#endif