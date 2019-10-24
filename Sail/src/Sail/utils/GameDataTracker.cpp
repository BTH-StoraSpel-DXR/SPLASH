#include "pch.h"
#include "GameDataTracker.h"
#include "Sail.h"
#include "../libraries/imgui/imgui.h"
#include <string>
#include "Network/NWrapperSingleton.h"

GameDataTracker::GameDataTracker() {
	m_loggedData = { 0 };
	m_network = &NWrapperSingleton::getInstance();
	m_placement = 13;
	m_nPlayersCurrentSession = 0;
}

GameDataTracker::~GameDataTracker() {

}

GameDataTracker& GameDataTracker::getInstance() {
	static GameDataTracker instance;

	return instance;
}

void GameDataTracker::init() {
	m_nPlayersCurrentSession = 0;
	for (auto player : m_network->getPlayers()) {
		m_hostPlayerTracker[player.id].nKills = 0;
		m_hostPlayerTracker[player.id].nDeaths = 0;
		m_hostPlayerTracker[player.id].placement = 1;
		m_nPlayersCurrentSession++;
	}
	m_placement = m_nPlayersCurrentSession + 1;
}

void GameDataTracker::resetData() {
	m_loggedData = { 0 };
	m_hostPlayerTracker.clear();
	m_placement = 13;
	m_nPlayersCurrentSession = 0;
}

void GameDataTracker::logWeaponFired() {
	m_loggedData.bulletsFired.QuadPart += 1;
}

void GameDataTracker::logEnemyHit() {
	m_loggedData.bulletsHit.QuadPart += 1;
}

void GameDataTracker::logEnemyKilled(Netcode::PlayerID playerID) {
	m_hostPlayerTracker[playerID].nKills += 1;
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

void GameDataTracker::logPlacement(Netcode::PlayerID playerID) {
	m_placement--;
	m_hostPlayerTracker[playerID].placement = m_placement;
}

const InduvidualStats& GameDataTracker::getStatistics() {
	return m_loggedData;
}

const std::vector<std::string>& GameDataTracker::getPlayerDeaths() {
	return m_playerDeaths;
}

const std::map<Netcode::PlayerID, HostStatsPerPlayer> GameDataTracker::getPlayerDataMap() {
	return m_hostPlayerTracker;
}

void GameDataTracker::setStatsForPlayer(Netcode::PlayerID id, int nKills, int placement) {
	m_hostPlayerTracker[id].nKills = nKills;
	m_hostPlayerTracker[id].placement = placement;
}

int GameDataTracker::getPlayerCount() {
	return m_nPlayersCurrentSession;
}

void GameDataTracker::renderImgui() {
	ImGui::Begin("Game Statistics", NULL);

	struct mapLayout {
		std::string name;
		int nKills;
	};
	std::map<int, mapLayout> tempPlacementMap;


	ImGui::Text("\n Placement------");

	/*for (auto player : m_hostPlayerTracker) {
		tempPlacementMap[player.second.placement].name = m_network->getPlayer((Netcode::PlayerID)player.first)->name;
		tempPlacementMap[player.second.placement].nKills = player.second.nKills;
	}

	int i = 1;
	for (auto player : tempPlacementMap) {
		std::string name = tempPlacementMap[i].name;

		std::string playerStats = name + " placed: " + std::to_string(i)
			+ " Kills: " + std::to_string(tempPlacementMap[i-1].nKills);

		ImGui::Text(playerStats.c_str());
		i++;
	}*/
	///-----------------------------------------------------------------------

	for (auto player : m_hostPlayerTracker) {
		tempPlacementMap[player.second.placement].name = m_network->getPlayer((Netcode::PlayerID)player.first)->name;
		tempPlacementMap[player.second.placement].nKills = player.second.nKills;
	}

	for (int i = 1; i < (int)tempPlacementMap.size() + 1; i++) {
		std::string name = tempPlacementMap[i].name;
		std::string playerStats = name + " placed: " + std::to_string(i)
			+ " Kills: " + std::to_string(tempPlacementMap[i].nKills);

		ImGui::Text(playerStats.c_str());
	}

	///-----------------------------------------------------------------------
	//
	//for (auto player : m_hostPlayerTracker) {
	//	std::string name = m_network->getPlayer((Netcode::PlayerID)player.first)->name;

	//	std::string playerStats = name + " placed: " + std::to_string(player.second.placement)
	//		+ " Kills: " + std::to_string(player.second.nKills);

	//	ImGui::Text(playerStats.c_str());
	//}

	ImGui::Text("\n Random stats------");

	ImGui::Text("Bullets Fired:");
	ImGui::Text(std::to_string(m_loggedData.bulletsFired.QuadPart).c_str());
	ImGui::Text("Bullets Hit:");
	ImGui::Text(std::to_string(m_loggedData.bulletsHit.QuadPart).c_str());
	ImGui::Text("Bullets Hit Percentage:");
	ImGui::Text(std::to_string(m_loggedData.bulletsHitPercentage.QuadPart).c_str());
	ImGui::Text("Distance Walked:");
	ImGui::Text(std::to_string(m_loggedData.distanceWalked).c_str());
	ImGui::Text("JumpsMade:");
	ImGui::Text(std::to_string(m_loggedData.jumpsMade.QuadPart).c_str());

		
	
	ImGui::End();
}
