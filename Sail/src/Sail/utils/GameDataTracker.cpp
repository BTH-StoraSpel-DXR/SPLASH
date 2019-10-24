#include "pch.h"
#include "GameDataTracker.h"
#include "Sail.h"
#include "../libraries/imgui/imgui.h"
#include <string>

GameDataTracker::GameDataTracker() {
	m_loggedData = { 0 };
}

GameDataTracker::~GameDataTracker() {

}

GameDataTracker& GameDataTracker::getInstance() {
	static GameDataTracker instance;

	return instance;
}

void GameDataTracker::resetData() {
	m_loggedData = { 0 };
}

void GameDataTracker::logWeaponFired() {
	m_loggedData.bulletsFired.QuadPart += 1;
}

void GameDataTracker::logEnemyHit() {
	m_loggedData.bulletsHit.QuadPart += 1;
}

void GameDataTracker::logEnemyKilled() {
	m_loggedData.enemiesKilled.QuadPart += 1;
}

void GameDataTracker::logJump() {
	m_loggedData.jumpsMade.QuadPart += 1;
}

void GameDataTracker::logDistanceWalked(glm::vec3 vector) {
	float distanceOfVector = sqrt(pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2));
	m_loggedData.distanceWalked += distanceOfVector;
}

void GameDataTracker::logPlayerDeath(const std::string& killer, const std::string& killed, const std::string& deathType) {
	m_playerDeaths.emplace_back(killer + " " + deathType + " " + killed);
}

const statistics& GameDataTracker::getStatistics() {
	return m_loggedData;
}

const std::vector<std::string>& GameDataTracker::getPlayerDeaths() {
	return m_playerDeaths;
}

void GameDataTracker::renderImgui() {
	ImGui::Begin("Game Statistics", NULL);

	ImGui::Text("Bullets Fired:");
	ImGui::Text(std::to_string(m_loggedData.bulletsFired.QuadPart).c_str());
	ImGui::Text("Bullets Hit:");
	ImGui::Text(std::to_string(m_loggedData.bulletsHit.QuadPart).c_str());
	ImGui::Text("Bullets Hit Percentage:");
	ImGui::Text(std::to_string(m_loggedData.bulletsHitPercentage.QuadPart).c_str());
	ImGui::Text("Enemies Killed:");
	ImGui::Text(std::to_string(m_loggedData.enemiesKilled.QuadPart).c_str());
	ImGui::Text("Distance Walked:");
	ImGui::Text(std::to_string(m_loggedData.distanceWalked).c_str());
	ImGui::Text("JumpsMade:");
	ImGui::Text(std::to_string(m_loggedData.jumpsMade.QuadPart).c_str());

	ImGui::End();
}
