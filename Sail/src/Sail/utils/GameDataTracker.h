#pragma once

#include <string>
#include <map>
#include "Sail/netcode/NetcodeTypes.h"

class NWrapperSingleton;

struct InduvidualStats {
	int bulletsFired;
	float distanceWalked;
	int jumpsMade;
};

struct GlobalTopStats {
	int bulletsFired;
	float distanceWalked;
	int jumpsMade;

	// IDs for the host to fill for the endscreen
	Netcode::PlayerID bfID;
	Netcode::PlayerID dwID;
	Netcode::PlayerID jmID;
};

struct HostStatsPerPlayer {
	int nKills = 0;
	int nDeaths = 0;
	int placement = 0;
};

class GameDataTracker {
public:
	//
	~GameDataTracker();

	// Implemented in...
	void logWeaponFired();						// ...ehfy::update()
	void logEnemyKilled(Netcode::PlayerID playerID);// CandleSystem::update
	void logJump();								// ...GameInputSystem::update()
	void logDistanceWalked(glm::vec3 vector);	// ...PhysicsSystem::update()
	void logPlayerDeath(const std::string& killer, const std::string& killed, const std::string& deathType); // used to log when a player is killed
	void logPlacement(Netcode::PlayerID playerID);

	void init();								// Gamestate::Gamestate()
	void resetData();							// EndGameState::onReturnToLobby() / renderImGui()
	// Filled with data at endgame to present to endscreen
	GlobalTopStats& getStatisticsGlobal();		// NetworkSenderSystem::writeEventToArchive()
	// Used to track the local player
	InduvidualStats& getStatisticsLocal();

	const std::vector<std::string>& getPlayerDeaths();

	const std::map<Netcode::PlayerID, HostStatsPerPlayer> getPlayerDataMap(); // NetworkSenderSystem

	// Used in end game when recieving player data stats
	void setStatsForPlayer(Netcode::PlayerID id, int nKills, int placement); // NetworkRecieverSystem::Update()
	void setStatsForOtherData(Netcode::PlayerID bfID, int bf,
		Netcode::PlayerID dwID, float dw, Netcode::PlayerID jmID, int jm);
	// nr of player from the start of the match
	int getPlayerCount();	// Nowhere atm
	void turnOffLocalDataTracking();

	// Implemented in...
	void renderImgui();							// ...EndState::renderImGui()

private:
	//
	NWrapperSingleton* m_network;

	InduvidualStats m_loggedData;
	GlobalTopStats m_loggedDataGlobal;
	bool m_trackLocalStats;				// toggle InduvidualStats tracking

	// Map of each player in current game containing data such as kills and deaths
	std::map<Netcode::PlayerID, HostStatsPerPlayer> m_hostPlayerTracker;
	int m_placement; // add 1 after ever time a player is placed to give next player a placement
	int m_nPlayersCurrentSession;

	std::vector<std::string> m_playerDeaths;

	struct mapLayout {
		std::string name;
		int nKills;
	};
	std::map<int, mapLayout> m_placementMap;
	void sort();
	bool m_sorted = false;

	// -+-+-+-+-+- Singleton requirements below -+-+-+-+-+-
public:
	GameDataTracker(GameDataTracker const&) = delete;
	void operator=(GameDataTracker const&) = delete;
	static GameDataTracker& getInstance();
private:
	GameDataTracker();
};