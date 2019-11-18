#include "pch.h"
#include "PlayerInfoWindow.h"

#include "Sail/Application.h"
#include "Sail/entities/ECS.h"
#include "Sail/graphics/camera/Camera.h"
#include "Sail/entities/components/MovementComponent.h"

PlayerInfoWindow::PlayerInfoWindow(bool showWindow) {}

PlayerInfoWindow::~PlayerInfoWindow() {}

void PlayerInfoWindow::setPlayerInfo(Entity* player, Camera* cam) {
	m_player = player;
	m_cam = cam;
}

void PlayerInfoWindow::renderWindow() {
	ImGui::Begin("Player information");
	if (m_player && m_cam) {
		std::string header;

		header = "Pos: " + Utils::toStr(m_cam->getPosition());
		ImGui::Text(header.c_str());

		header = "Dir: " + Utils::toStr(m_cam->getDirection());
		ImGui::Text(header.c_str());

		if (m_player->getComponent<MovementComponent>()) {
			header = "Vel: " + Utils::toStr(m_player->getComponent<MovementComponent>()->velocity);
			ImGui::Text(header.c_str());

			header = "relative Vel: " + Utils::toStr(m_player->getComponent<MovementComponent>()->relVel);
			ImGui::Text(header.c_str());
		}
	}
	ImGui::End();
}
