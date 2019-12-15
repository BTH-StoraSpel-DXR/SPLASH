#include "pch.h"
#include "GameDataTracker.h"
#include "Sail.h"
#include "../libraries/imgui/imgui.h"
#include <string>
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/SailImGui/SailImGui.h"

GameDataTracker::GameDataTracker() {
	m_loggedData = { 0 };
	m_loggedDataGlobal = { 0 };
	m_network = &NWrapperSingleton::getInstance();
	m_placement = 13;
	m_nPlayersCurrentSession = 0;
	m_trackLocalStats = true;
	m_app = Application::getInstance();
	
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_DISCONNECT, this);
}

GameDataTracker::~GameDataTracker() {
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DISCONNECT, this);
}

GameDataTracker& GameDataTracker::getInstance() {
	static GameDataTracker instance;

	return instance;
}

void GameDataTracker::init() {
	resetData();
}

void GameDataTracker::resetData() {
	m_trackLocalStats = true;
	m_loggedData = { 0 };
	m_loggedDataGlobal = { 0 };
	m_killFeed.clear();

	m_loggedDataGlobal.bulletsFiredID = 0;
	m_loggedDataGlobal.distanceWalkedID = 0;
	m_loggedDataGlobal.jumpsMadeID = 0;
	m_hostPlayerTracker.clear();

	m_torchesLeft = 3;
	m_playersLeft = 0;
	m_nPlayersCurrentSession = 0;
	for (auto player : m_network->getPlayers()) {

		if (player.team != -1) {
			m_hostPlayerTracker[player.id].nKills = 0;
			m_hostPlayerTracker[player.id].nDeaths = 0;
			m_hostPlayerTracker[player.id].placement = 13;
			m_hostPlayerTracker[player.id].damage = 0;
			m_hostPlayerTracker[player.id].damageTaken = 0;

			m_hostPlayerTracker[player.id].playerName = player.name;
			m_nPlayersCurrentSession++;
		}
		
	}
	m_placement = m_nPlayersCurrentSession + 1;
}

void GameDataTracker::logWeaponFired() {
	if (m_trackLocalStats) {
		m_loggedData.bulletsFired += 1;
	}
}

void GameDataTracker::logEnemyKilled(Netcode::PlayerID killer) {
	m_hostPlayerTracker[killer].nKills += 1;
}

void GameDataTracker::logDeath(Netcode::PlayerID victim) {
	m_hostPlayerTracker[victim].nDeaths += 1;
}

void GameDataTracker::logDamageDone(Netcode::PlayerID playerID, const int dmg) {
	m_hostPlayerTracker[playerID].damage += dmg;
}
void GameDataTracker::logDamageTaken(Netcode::PlayerID playerID, const int dmg) {
	m_hostPlayerTracker[playerID].damageTaken += dmg;
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

void GameDataTracker::logPlacement(Netcode::PlayerID playerID) {
	m_placement--;
	m_hostPlayerTracker[playerID].placement = m_placement;
}

void GameDataTracker::logMessage(const std::string& message) {
	m_killFeed.push_back(message);
}

GlobalTopStats& GameDataTracker::getStatisticsGlobal() {
	return m_loggedDataGlobal;
}

InduvidualStats& GameDataTracker::getStatisticsLocal() {
	return m_loggedData;
}

const std::vector<std::string>& GameDataTracker::getKillFeed() {
	return m_killFeed;
}

const std::map<Netcode::PlayerID, HostStatsPerPlayer> GameDataTracker::getPlayerDataMap() {
	return m_hostPlayerTracker;
}

void GameDataTracker::setStatsForPlayer(Netcode::PlayerID id, int nKills, int placement, int nDeaths, int damage, int damageTaken) {
	m_hostPlayerTracker[id].placement = placement;
	m_hostPlayerTracker[id].nKills = nKills;
	m_hostPlayerTracker[id].nDeaths = nDeaths;
	m_hostPlayerTracker[id].damage = damage;
	m_hostPlayerTracker[id].damageTaken = damageTaken;
}

void GameDataTracker::setStatsForOtherData(Netcode::PlayerID bfID, int bf, Netcode::PlayerID dwID, float dw, Netcode::PlayerID jmID, int jm) {
	m_loggedDataGlobal.bulletsFiredID = bfID;
	m_loggedDataGlobal.bulletsFired = bf;
	m_loggedDataGlobal.distanceWalkedID = dwID;
	m_loggedDataGlobal.distanceWalked = dw;
	m_loggedDataGlobal.jumpsMadeID = jmID;
	m_loggedDataGlobal.jumpsMade = jm;
}

int GameDataTracker::getPlayerCount() {
	return m_nPlayersCurrentSession;
}

void GameDataTracker::turnOffLocalDataTracking() {
	m_trackLocalStats = false;
}

void GameDataTracker::renderImgui() {

	



	


}

void GameDataTracker::renderPlacement() {
	static ImGuiHandler* handler = m_app->getImGuiHandler();

	std::map<int, int> tempPlacementMap;

	// Sort rankings
	for (auto& [key, value]: m_hostPlayerTracker) {
		tempPlacementMap[value.placement] = key;
	}


	static float a[4] = {
		0.4f,
		0.61f,
		0.82f,
		1.0f
	};
	//KEEP
	//coImGui::SliderFloat4("##DEBUG0", &a[0], 0.0f, 1.0f);

	float x[4] = {
		ImGui::GetWindowContentRegionWidth()*a[0],
		ImGui::GetWindowContentRegionWidth()*a[1],
		ImGui::GetWindowContentRegionWidth()*a[2],
		ImGui::GetWindowContentRegionWidth()*a[3]
	};

	//ImGui::PushFont(handler->getFont("Beb40"));
	ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("Header2"));
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	SailImGui::cText("SCOREBOARD", ImGui::GetWindowContentRegionWidth());
	ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));
	ImGui::PopStyleColor();
	//ImGui::PopFont();
	ImGui::Separator();
	std::string gamemode = m_app->getSettings().gameSettingsStatic["gamemode"]["types"].getSelected().name;
	std::string map = m_app->getSettings().defaultMaps[gamemode].getSelected().name; 

	ImGui::Text(map.c_str());
	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetCursorPosX() + 30.0f);
	ImGui::Text("|");
	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetCursorPosX() + 30.0f);

	ImGui::Text(gamemode.c_str());
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::SameLine(x[0]);
	SailImGui::rText("Extinguishes",8);
	ImGui::SameLine(x[1]);
	SailImGui::rText("Torches Lost",8);
	ImGui::SameLine(x[2]);
	SailImGui::rText("Damage",8);
	ImGui::SameLine(x[3]);
	SailImGui::rText("Damage Taken",8);
	ImGui::Spacing();

	if (ImGui::BeginChild("##SCOREBOARD", ImVec2(0, 0))) {
		// key = placement, value = index in playerlist
		ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("smalltext"));
		for (auto& [key, value] : tempPlacementMap) {
			Player* player = NWrapperSingleton::getInstance().getPlayer(value);

			bool me = player ? (*player == NWrapperSingleton::getInstance().getMyPlayer()):false;
			///PLAYER NAME
			//Player still in session
			if (player) {
				
				if (me) {
					ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));
					//ImGui::PushFont(handler->getFont("Beb24"));
				}
				glm::vec3 tempC(m_app->getSettings().getColor(m_app->getSettings().teamColorIndex(player->team)));
				ImVec4 col(
					tempC.x,
					tempC.y,
					tempC.z,
					1
				);
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				ImGui::Text(std::string(m_hostPlayerTracker[value].playerName + (me?"*":"")).c_str());
				ImGui::PopStyleColor();
		
			}
			//player not in session
			else {
				ImGui::Text(m_hostPlayerTracker[value].playerName.c_str());
			}

			// EXTINGUISHES
			ImGui::SameLine(x[0]);
			SailImGui::rText(std::to_string(m_hostPlayerTracker[value].nKills).c_str(), 0);
			//TORCHES LOST
			ImGui::SameLine(x[1]);
			SailImGui::rText(std::to_string(m_hostPlayerTracker[value].nDeaths).c_str(), 0);
			//DAMAGE DONE
			ImGui::SameLine(x[2]);
			SailImGui::rText(std::to_string(m_hostPlayerTracker[value].damage).c_str(), 0);
			//DAMAGE TAKEN
			ImGui::SameLine(x[3]);
			SailImGui::rText(std::to_string(m_hostPlayerTracker[value].damageTaken).c_str(), 0);

			if (me) {
				//ImGui::PopFont();
				ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("smalltext"));

			}
		}
	}
	ImGui::EndChild();
}

void GameDataTracker::renderPersonalStats() {



	std::string localStatsString = "water wasted: " + std::to_string(m_loggedData.bulletsFired) + "L";
	ImGui::Text(localStatsString.c_str());

	localStatsString = "Distance walked: " + std::to_string((int)m_loggedData.distanceWalked) + "m";
	ImGui::Text(localStatsString.c_str());

	localStatsString = "Jumps made: " + std::to_string(m_loggedData.jumpsMade);
	ImGui::Text(localStatsString.c_str());
}

void GameDataTracker::renderFunStats() {
	std::string bdString = "Most water wasted by " +
		m_hostPlayerTracker[m_loggedDataGlobal.bulletsFiredID].playerName + ": " + std::to_string(m_loggedDataGlobal.bulletsFired) +"L";
	ImGui::Text(bdString.c_str());

	bdString = "Longest distance walked by " +
		m_hostPlayerTracker[m_loggedDataGlobal.distanceWalkedID].playerName + ": " + std::to_string((int)m_loggedDataGlobal.distanceWalked) + "m";
	ImGui::Text(bdString.c_str());

	bdString = "Most jumps made by " +
		m_hostPlayerTracker[m_loggedDataGlobal.jumpsMadeID].playerName + ": " + std::to_string(m_loggedDataGlobal.jumpsMade);
	ImGui::Text(bdString.c_str());
}

void GameDataTracker::renderWinners() {




}

#ifdef DEVELOPMENT
void GameDataTracker::addDebugData() {
	for (int i = -1; i > -10; i--) {
		m_hostPlayerTracker[i].nKills = i*-1;
		m_hostPlayerTracker[i].placement = i*-1;
		m_hostPlayerTracker[i].playerName = std::string("player"+std::to_string(i));
		m_nPlayersCurrentSession++;
	}
}
#endif

const int GameDataTracker::getTorchesLeft() {
	return m_torchesLeft;
}

void GameDataTracker::reduceTorchesLeft() {
	m_torchesLeft--;
}

const int GameDataTracker::getPlayersLeft() {
	return m_playersLeft;
}
void GameDataTracker::setPlayersLeft(int playersLeft) {
	m_playersLeft = playersLeft;
}

bool GameDataTracker::onEvent(const Event& e) {

	switch (e.type) {
	case Event::Type::NETWORK_DISCONNECT:		playerDisconnected((const NetworkDisconnectEvent&)e); break;
	default:
		break;
	}

	return true;
}

void GameDataTracker::playerDisconnected(const NetworkDisconnectEvent& e) {
	if (m_trackLocalStats) {
		logPlacement(e.player.id);
	}
}
