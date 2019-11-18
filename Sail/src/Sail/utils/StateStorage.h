#pragma once

#include <string>
#include <list>

struct MenuToLobbyData {
	std::string name;
};

struct ToGameData {
	int botCount = 0;
	int team = 1; // team 0 = spectator

	ToGameData() {}
	ToGameData(int nrOfBots, int _team) : botCount(nrOfBots), team(_team) {}
};

class StateStorage {
public:
	StateStorage() {
		// Initialize private data to 0 //
		m_menuLobbyData.name = "";
	}
	~StateStorage() { }

	void setMenuToLobbyData(MenuToLobbyData& data) { m_menuLobbyData = data; }
	const MenuToLobbyData* getMenuToLobbyData() { return &m_menuLobbyData; }

	void setLobbyToGameData(ToGameData& data) { m_lobyToGameData = data; }
	const ToGameData* getLobbyToGameData() { return &m_lobyToGameData; }


private:
	MenuToLobbyData m_menuLobbyData;
	ToGameData m_lobyToGameData;

};