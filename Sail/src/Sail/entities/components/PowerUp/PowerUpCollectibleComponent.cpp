#include "pch.h"
#include "PowerUpCollectibleComponent.h"
#include "../ModelComponent.h"

PowerUpCollectibleComponent::PowerUpCollectibleComponent() :
	powerUp(0),
	powerUpDuration(15.0f),
	respawnTime(-1),
	time(0.0f)
{
	
}

PowerUpCollectibleComponent::~PowerUpCollectibleComponent() {
}
#ifdef DEVELOPMENT
void PowerUpCollectibleComponent::imguiRender(Entity** selected) {
	ImGui::Columns(2, nullptr, false);
	ImGui::Text("Type");
	std::vector<std::string> names = { "RUNSPEED", "STAMINA", "SHOWER", "POWERWASH" };
	if (ImGui::BeginCombo("##POWERUPCOMBO", names[powerUp].c_str())) {
		int i = 0;
		for (auto& name : names) {
			if (ImGui::Selectable(name.c_str(), powerUp == i)) {
				(*selected)->getComponent<ModelComponent>()->teamColorID = i;
				powerUp = i;
			}
			i++;
		}
		ImGui::EndCombo();
	}
	ImGui::NextColumn();
	ImGui::Text("Duration");
	ImGui::SliderFloat("##DURATION", &powerUpDuration, 0, 30);
	ImGui::NextColumn();
	ImGui::Text("respawnTime");
	ImGui::NextColumn();
	ImGui::SliderFloat("##RESPAWNTIME", &respawnTime, -1.0f, 120.0f);
	ImGui::NextColumn();
	ImGui::Text("Time left");
	ImGui::NextColumn();
	ImGui::SliderFloat("##TIMELEFT", &time, 0, respawnTime);
	ImGui::Columns(1);
}
#endif