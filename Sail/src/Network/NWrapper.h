#pragma once

#include "Network/NetworkStructs.hpp"
#include "../../SPLASH/src/game/states/LobbyState.h"	// Needed for 'Message'
#include "Sail.h"
#include "Sail/netcode/NetcodeTypes.h"

#include <string>

#include "Sail/states/StateIdentifiers.h"


#define MAX_NAME_LENGTH 100


class Network;

struct StateStatus {
	StateStatus() {
		state = (States::ID)0;
		status = 0;
	}
	States::ID state;
	char status;
};


enum class PlayerLeftReason : char {
		CONNECTION_LOST = 0,
		KICKED
};

// Move elsewhere?
struct Player {
	Netcode::PlayerID id;
	std::string name;
	char team = 0;
	bool justJoined = true;
	StateStatus lastStateStatus;

	Player(Netcode::PlayerID setID = HOST_ID, std::string setName = "Hans")
		: name(setName), id(setID)
	{
		name.reserve(MAX_NAME_LENGTH);
	}

	bool friend operator==(const Player& left, const Player& right) {
		if (left.id == right.id &&
			left.name == right.name) {
			return true;
		}
		return false;
	}
};

struct GameOnLanDescription {
	std::string ip;
	USHORT port;
	std::string name;
	unsigned char nPlayers;
	unsigned char maxPlayers;
	States::ID currentState;
};

struct Message {
	Netcode::PlayerID senderID;
	std::string content;
};

class NWrapper : public NetworkEventHandler {
public:
	NWrapper(Network* pNetwork);
	virtual ~NWrapper();

	virtual bool host(int port = 54000) = 0;
	virtual bool connectToIP(char* = "127.0.0.1:54000") = 0;

	virtual void sendChatMsg(std::string msg) = 0;

	void sendSerializedDataAllClients(const std::string& data);
	void sendSerializedDataToHost(const std::string& data);
	virtual void sendSerializedDataToClient(const std::string& data, Netcode::PlayerID PlayerId) = 0;

	/*
		Host Only
			
		This will request a clients to enter a new state. GameState, EndGameState etc.
		playerId == 255 will send to all
	*/
	virtual void setClientState(States::ID state, Netcode::PlayerID playerId = 255) {};
	virtual void kickPlayer(Netcode::PlayerID playerId) {};
	virtual void updateGameSettings(std::string s) {};
	virtual void updateStateLoadStatus(States::ID state, char status) = 0;

	virtual void requestTeam(char team) {};
	virtual void setTeamOfPlayer(char team, Netcode::PlayerID playerID, bool dispatch = true) {};
	virtual void requestTeamColor(char teamColorID) {};
protected:
	enum MessageLetter : char {
		ML_NULL = 0,
		ML_CHAT,
		ML_DISCONNECT,
		ML_JOIN,
		ML_NAME_REQUEST,
		ML_WELCOME,
		ML_SERIALIZED,
		ML_CHANGE_STATE,
		ML_UPDATE_STATE_LOAD_STATUS,
		ML_UPDATE_SETTINGS,
		ML_TEAM_REQUEST,
		ML_TEAMCOLOR_REQUEST,
	};

protected:
	void sendMsg(const char* msg, size_t size, TCP_CONNECTION_ID tcp_id = 0);
	void sendMsgAllClients(const char* msg, size_t size);
		
	Network* m_network = nullptr;
	Application* m_app = nullptr;	

	// Parsing functions | Will alter 'data' upon being used.
	TCP_CONNECTION_ID parseID(std::string& data);	//
	std::string parseName(std::string& data);		//
	Message processChatMessage(const char* data);
	States::ID m_lastReportedState;

private:
	friend class NWrapperSingleton;
	void initialize(Network* pNetwork);
	void handleNetworkEvents(NetworkEvent nEvent);

	virtual void playerJoined(TCP_CONNECTION_ID id) = 0;
	virtual void playerDisconnected(TCP_CONNECTION_ID id) = 0;
	virtual void playerReconnected(TCP_CONNECTION_ID id) = 0;
	virtual void decodeMessage(NetworkEvent nEvent) = 0;
};