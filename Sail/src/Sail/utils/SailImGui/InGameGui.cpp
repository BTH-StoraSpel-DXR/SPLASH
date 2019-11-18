#include "pch.h"
#include "InGameGui.h"
#include "Sail/Application.h"
#include "Sail/utils/SailImGui/CustomImGuiComponents/CustomImGuiComponents.h"

#include "Sail/entities/components/SanityComponent.h"
#include "Sail/entities/components/SprintingComponent.h"

InGameGui::InGameGui(bool showWindow) {
}

InGameGui::~InGameGui() {
}

void InGameGui::renderWindow() {
	float screenWidth = Application::getInstance()->getWindow()->getWindowWidth();
	float screenHeight = Application::getInstance()->getWindow()->getWindowHeight();
	float progresbarLenght = 300;
	float progresbarHeight = 40;
	float outerPadding = 50;

	ImGui::SetNextWindowPos(ImVec2(
		screenWidth - progresbarLenght,
		screenHeight - progresbarHeight * 3
	));

	ImGui::SetNextWindowSize(ImVec2(
		progresbarLenght,
		progresbarHeight - progresbarHeight * 2.2
	));

	ImGui::Begin("GUI", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

	if (m_player) {

		SanityComponent* c1 = m_player->getComponent<SanityComponent>();
		SprintingComponent* c2 = m_player->getComponent<SprintingComponent>();

		if (c1) {
			float val = c1->sanity / 100.f;
			float val_inv = 1 - val;

			CustomImGui::CustomProgressBar(val, ImVec2(-1, 0), "Sanity", ImVec4(1 - val_inv * 0.3, 0.6 - val_inv * 0.6, 0, 1));
		}

		if (c2) {
			float val;
			float val_inv;
			ImVec4 color;

			if (c2->exhausted) {
				val = (c2->downTimer / MAX_SPRINT_DOWN_TIME);
				val_inv = 1 - val;
				color = ImVec4(0.5, 0.5, 0.5, 1);
			} else {
				val = 1 - (c2->sprintTimer / MAX_SPRINT_TIME);
				val_inv = 1 - val;
				color = ImVec4(1 - val_inv * 0.3, 0.6 - val_inv * 0.6, 0, 1);
			}


			CustomImGui::CustomProgressBar(val, ImVec2(-1, 0), "Stamina", color);
		}
	}

	ImGui::End();
}

void InGameGui::setPlayer(Entity* player) {
	m_player = player;
}
