#include "pch.h"
#include "GameDataTracker.h"
#include "Sail.h"
#include "../libraries/imgui/imgui.h"
#include <string>
#include "Network/NWrapperSingleton.h"

GameDataTracker::GameDataTracker() {
	m_loggedData = { 0 };
	m_loggedDataGlobal = { 0 };
	m_network = &NWrapperSingleton::getInstance();
	m_placement = 13;
	m_nPlayersCurrentSession = 0;
	m_trackLocalStats = true;
}

GameDataTracker::~GameDataTracker() {

}

void GameDataTracker::sort() {
	// Sort rankings
	for (auto player : m_hostPlayerTracker) {
		m_placementMap[player.second.placement].name = m_network->getPlayer((Netcode::PlayerID)player.first)->name;
		m_placementMap[player.second.placement].nKills = player.second.nKills;
	}

	m_sorted = true;
}

GameDataTracker& GameDataTracker::getInstance() {
	static GameDataTracker instance;

	return instance;
}

void GameDataTracker::init() {

	m_trackLocalStats = true;
	m_loggedData = { 0 };
	m_loggedDataGlobal = { 0 };

	m_loggedDataGlobal.bfID = NWrapperSingleton::getInstance().getMyPlayerID();
	m_loggedDataGlobal.dwID = NWrapperSingleton::getInstance().getMyPlayerID();
	m_loggedDataGlobal.jmID = NWrapperSingleton::getInstance().getMyPlayerID();

	if (NWrapperSingleton::getInstance().isHost()) {
		m_nPlayersCurrentSession = 0;
		for (auto player : m_network->getPlayers()) {
			m_hostPlayerTracker[player.id].nKills = 0;
			m_hostPlayerTracker[player.id].nDeaths = 0;
			m_hostPlayerTracker[player.id].placement = 1;
			m_nPlayersCurrentSession++;
		}
		m_placement = m_nPlayersCurrentSession + 1;
	}
}

void GameDataTracker::resetData() {
	m_loggedData = { 0 };
	m_loggedDataGlobal = { 0 };
	m_hostPlayerTracker.clear();
	m_placement = 13;
	m_nPlayersCurrentSession = 0;
}

void GameDataTracker::logWeaponFired() {
	if (m_trackLocalStats) {
		m_loggedData.bulletsFired += 1;
	}
	
}

void GameDataTracker::logEnemyKilled(Netcode::PlayerID playerID) {
	m_hostPlayerTracker[playerID].nKills += 1;
}

void GameDataTracker::logJump() {
	if (m_trackLocalStats) {
		m_loggedData.jumpsMade += 1;
	}
}

void GameDataTracker::logDistanceWalked(glm::vec3 vector) {
	if (m_trackLocalStats) {
		float distanceOfVector = sqrt(pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2));
		m_loggedData.distanceWalked += distanceOfVector;
	}
	
}

void GameDataTracker::logPlayerDeath(const std::string& killer, const std::string& killed, const std::string& deathType) {
	m_playerDeaths.emplace_back(killer + " " + deathType + " " + killed);
}

void GameDataTracker::logPlacement(Netcode::PlayerID playerID) {
	m_placement--;
	m_hostPlayerTracker[playerID].placement = m_placement;
}

GlobalTopStats& GameDataTracker::getStatisticsGlobal() {
	return m_loggedDataGlobal;
}

InduvidualStats& GameDataTracker::getStatisticsLocal() {
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

void GameDataTracker::setStatsForOtherData(Netcode::PlayerID bfID, int bf, Netcode::PlayerID dwID, float dw, Netcode::PlayerID jmID, int jm) {
	m_loggedDataGlobal.bfID = bfID;
	m_loggedDataGlobal.bulletsFired = bf;
	m_loggedDataGlobal.dwID = dwID;
	m_loggedDataGlobal.distanceWalked = dw;
	m_loggedDataGlobal.jmID = jmID;
	m_loggedDataGlobal.jumpsMade = jm;
}

int GameDataTracker::getPlayerCount() {
	return m_nPlayersCurrentSession;
}

void GameDataTracker::turnOffLocalDataTracking() {
	m_trackLocalStats = false;
}

void GameDataTracker::renderImgui() {

	ImGui::Begin("Game Statistics", NULL);
	ImGui::SetWindowPos({ 18,12 });
	ImGui::SetWindowSize({ 307,600 });

	// Sort if not sorted.
	if (!m_sorted) {
		sort();
	}

	ImGui::Text("\n Placement------");


	for (int i = 1; i < (int)m_placementMap.size() + 1; i++) {
		std::string name = m_placementMap[i].name;
		std::string playerStats = name + " placed: " + std::to_string(i)
			+ " Kills: " + std::to_string(m_placementMap[i].nKills);

		ImGui::Text(playerStats.c_str());
	}

	ImGui::Text("\n Misc stats------");
	
	std::string bdString = "Most bullets fires by " + 
		m_network->getPlayer(m_loggedDataGlobal.bfID)->name + ": " + std::to_string(m_loggedDataGlobal.bulletsFired);
	ImGui::Text(bdString.c_str());

	bdString = "Longest distance walked by " +
		m_network->getPlayer(m_loggedDataGlobal.dwID)->name + ": " + std::to_string((int)m_loggedDataGlobal.distanceWalked) + "m";
	ImGui::Text(bdString.c_str());

	bdString = "Most jumps made by " +
		m_network->getPlayer(m_loggedDataGlobal.jmID)->name + ": " + std::to_string(m_loggedDataGlobal.jumpsMade);
	ImGui::Text(bdString.c_str());
	
	ImGui::End();



	ImGui::Begin("WINNER", NULL);
	ImGui::SetWindowFontScale(5.0f);
	ImGui::SetWindowPos({331,12});
	ImGui::SetWindowSize({ 404,292 });
	ImGui::Text(m_placementMap[1].name.c_str());
	ImGui::End();

	ImGui::Begin("Personal Statistics", NULL);
	ImGui::SetWindowPos({ 331,310 });
	ImGui::SetWindowSize({ 307,293 });

	std::string localStatsString = "Bullets fired: " + std::to_string(m_loggedData.bulletsFired);
	ImGui::Text(localStatsString.c_str());

	localStatsString = "Distance walked: " + std::to_string((int)m_loggedData.distanceWalked) + "m";
	ImGui::Text(localStatsString.c_str());

	localStatsString = "Jumps made: " + std::to_string(m_loggedData.jumpsMade);
	ImGui::Text(localStatsString.c_str());

	ImGui::End();
}
