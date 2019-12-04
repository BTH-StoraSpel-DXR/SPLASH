#include "pch.h"
#include "NWrapperClient.h"
#include "Network/NetworkModule.hpp"

#include "Sail/events/EventDispatcher.h"

#include "Sail/../Network/NWrapperSingleton.h"
#include "../../SPLASH/src/game/events/NetworkChatEvent.h"
#include "../../SPLASH/src/game/events/NetworkNameEvent.h"
#include "../../SPLASH/src/game/events/NetworkDroppedEvent.h"
#include "../../SPLASH/src/game/events/NetworkSerializedPackageEvent.h"
#include "../../SPLASH/src/game/events/SettingsEvent.h"
#include "../../SPLASH/src/game/states/LobbyState.h"
#include "../../SPLASH/src/game/events/SettingsEvent.h"
#include "Sail/events/types/NetworkUpdateStateLoadStatus.h"
#include "Sail/events/types/NetworkPlayerChangedTeam.h"


bool NWrapperClient::host(int port) {
	// A client does not host, do nothing.
	return false;
}

bool NWrapperClient::connectToIP(char* adress) {
	bool adressIsIP = true;
	char IP[15] = { 0 };
	char portChar[5] = { 0 };
	int port = 0;

	int ipCounter = 0;
	int charCounter = 0;

	for (size_t i = 0; i < 22; i++)
	{
		if (adress[i] == ':')
		{
			i++;
			adressIsIP = false;
		}
		if (adressIsIP)
		{
			IP[ipCounter] = adress[i];
			ipCounter++;
		}
		else
		{
			if (charCounter >= sizeof(portChar))
			{
				continue;
			}
			portChar[charCounter] = adress[i];
			charCounter++;
		}
	}

	port = std::atoi(portChar);

	return m_network->join(IP, port);
}

void NWrapperClient::sendChatMsg(std::string msg) {
	std::string data;
	data += ML_CHAT;
	data += NWrapperSingleton::getInstance().getMyPlayer().id;
	data += msg;
	m_network->send(data.c_str(), data.length() + 1);
}

void NWrapperClient::playerJoined(TCP_CONNECTION_ID id) {
	// Clients never have others connecting to them.
}

void NWrapperClient::playerDisconnected(TCP_CONNECTION_ID id_) {
	// My connection to host was lost :(	
	EventDispatcher::Instance().emit(NetworkDroppedEvent());
}

void NWrapperClient::playerReconnected(TCP_CONNECTION_ID id) {
	// Connection with host re-established, not implemented yet.
}

void NWrapperClient::decodeMessage(NetworkEvent nEvent) {
	// These will be assigned in the switch case.
	unsigned char userID;
	std::list<Player> playerList;	// Only used in 'w'-case but needs to be initialized up here
	Player currentPlayer{};
	int charCounter = 0;
	std::string id_string = "";	
	std::string remnants = "";
	unsigned int id_number = 0;	
	std::string id = "";
	std::string remnants_m = "";
	unsigned char id_question;
	Message processedMessage;
	std::string dataString;
	bool isSpectator = false;

	switch (nEvent.data->Message.rawMsg[0]) {
	case ML_CHAT:
		// Client received a chat message from the host...
		// Parse and process into Message struct
		processedMessage = processChatMessage(&nEvent.data->Message.rawMsg[1]);

		// Dispatch Message to lobby.
		EventDispatcher::Instance().emit(NetworkChatEvent(processedMessage));

		break;

	case ML_DISCONNECT:
		// A player disconnected from the host...
		// Get the user ID from the event data.		
	{
		userID = nEvent.data->Message.rawMsg[1];
		PlayerLeftReason reason = (PlayerLeftReason)nEvent.data->Message.rawMsg[2];
		NWrapperSingleton::getInstance().playerLeft(userID, true, reason);
	}
		break;
	case ML_JOIN:
		currentPlayer.id = (unsigned char)nEvent.data->Message.rawMsg[1];
		currentPlayer.name = &nEvent.data->Message.rawMsg[2];
		NWrapperSingleton::getInstance().playerJoined(currentPlayer);
		break;
	case ML_NAME_REQUEST:
		id_question = (unsigned char)nEvent.data->Message.rawMsg[1]; //My player ID that the host just have given me.
		NWrapperSingleton::getInstance().getMyPlayer().id = id_question;
		sendMyNameToHost();

		break;
	case ML_UPDATE_STATE_LOAD_STATUS:
	{
		Netcode::PlayerID playerID = (Netcode::PlayerID)nEvent.data->Message.rawMsg[1];
		States::ID state = (States::ID)nEvent.data->Message.rawMsg[2];
		char status = nEvent.data->Message.rawMsg[3];

		if (playerID == NWrapperSingleton::getInstance().getMyPlayerID()) {
			break;
		}

		Player* player = NWrapperSingleton::getInstance().getPlayer(playerID);
		player->lastStateStatus.state = state;
		player->lastStateStatus.status = status;

		EventDispatcher::Instance().emit(NetworkUpdateStateLoadStatus(state, playerID, status));
	}
		break;
	case ML_CHANGE_STATE:
	{
		States::ID stateID = (States::ID)(nEvent.data->Message.rawMsg[1]);

		EventDispatcher::Instance().emit(NetworkChangeStateEvent(stateID));

	}
	break;
	case ML_WELCOME:
		// The host has sent us a welcome-package with a list of the players in the game...
		// Parse the welcome-package into a list of players
		remnants = nEvent.data->Message.rawMsg;
		while (remnants != "") {
			currentPlayer.id = this->parseID(remnants);
			currentPlayer.name = this->parseName(remnants);

			// Only add players with full names.
			if (currentPlayer.name != "") {
				playerList.push_back(currentPlayer);
			}
		}

		updatePlayerList(playerList);

		break;
	case ML_SERIALIZED: // Serialized data, remove first character and send the rest to be deserialized
		dataString = std::string(nEvent.data->Message.rawMsg, nEvent.data->Message.sizeOfMsg);
		dataString.erase(0, 1); // remove the s

		// Send the serialized stringData as an event to the networkSystem which parses it.
		EventDispatcher::Instance().emit(NetworkSerializedPackageEvent(dataString));
		break;

	case ML_UPDATE_SETTINGS:
	{
		auto& stat = m_app->getSettings().gameSettingsStatic;
		auto& dynamic = m_app->getSettings().gameSettingsDynamic;
		m_app->getSettings().deSerialize(std::string(&nEvent.data->Message.rawMsg[1]), stat, dynamic);
		EventDispatcher::Instance().emit(SettingsUpdatedEvent());
	}
		break;
	case ML_TEAM_REQUEST:
	{
		char team = nEvent.data->Message.rawMsg[1];
		Netcode::PlayerID playerID = nEvent.data->Message.rawMsg[2];
		bool dispatch = nEvent.data->Message.rawMsg[3];

		Player* p = NWrapperSingleton::getInstance().getPlayer(playerID);
		if(p){
			p->team = team;
			if (dispatch) {
				EventDispatcher::Instance().emit(NetworkPlayerChangedTeam(p->id));
			}
		}
	}
		break;
	default:
		break;
	}
}

void NWrapperClient::sendMyNameToHost() {
	// Save the ID which the host has blessed us with
	std::string message;
	message += ML_NAME_REQUEST;
	message += NWrapperSingleton::getInstance().getMyPlayerName().c_str();
	sendMsg(message.c_str(), message.length() + 1);
}

void NWrapperClient::updatePlayerList(std::list<Player>& playerList) {
	if (playerList.size() >= 2) {
		NWrapperSingleton::getInstance().resetPlayerList();

		for (auto currentPlayer : playerList) {
			NWrapperSingleton::getInstance().playerJoined(currentPlayer);
		}
	}
}

void NWrapperClient::requestTeam(char team) {
	char msg[] = { ML_TEAM_REQUEST, team, ML_NULL };
	sendMsg(msg, sizeof(msg));
}

void NWrapperClient::requestTeamColor(char teamColorID) {
	char msg[] = { ML_TEAMCOLOR_REQUEST, teamColorID, ML_NULL };
	sendMsg(msg, sizeof(msg));
}

void NWrapperClient::updateStateLoadStatus(States::ID state, char status) {
	m_lastReportedState = state;

	Player* myPlayer = NWrapperSingleton::getInstance().getPlayer(NWrapperSingleton::getInstance().getMyPlayerID());
	myPlayer->lastStateStatus.state = state;
	myPlayer->lastStateStatus.status = status;

	char msg[] = { ML_UPDATE_STATE_LOAD_STATUS, NWrapperSingleton::getInstance().getMyPlayerID(), state, status, ML_NULL };

	sendMsg(msg, sizeof(msg));
}
