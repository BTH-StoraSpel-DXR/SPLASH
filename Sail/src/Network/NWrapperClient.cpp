#include "pch.h"
#include "NWrapperClient.h"
#include "Network/NetworkModule.hpp"

#include "Sail/events/EventDispatcher.h"

#include "Sail/../Network/NWrapperSingleton.h"
#include "../../SPLASH/src/game/events/NetworkDisconnectEvent.h"
#include "../../SPLASH/src/game/events/NetworkChatEvent.h"
#include "../../SPLASH/src/game/events/NetworkNameEvent.h"
#include "../../SPLASH/src/game/events/NetworkDroppedEvent.h"
#include "../../SPLASH/src/game/events/NetworkSerializedPackageEvent.h"
#include "../../SPLASH/src/game/states/LobbyState.h"
#include "../../SPLASH/src/game/events/SettingsEvent.h"
#include "../../SPLASH/src/game/events/NetworkSwitchState.h"

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
	std::string data = "m";
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

	switch (nEvent.data->Message.rawMsg[0]) {
	case 'm':
		// Client received a chat message from the host...
		// Parse and process into Message struct
		processedMessage = processChatMessage(&nEvent.data->Message.rawMsg[1]);

		// Dispatch Message to lobby.
		EventDispatcher::Instance().emit(NetworkChatEvent(processedMessage));

		break;

	case 'd':
		// A player disconnected from the host...
		// Get the user ID from the event data.
		userID = nEvent.data->Message.rawMsg[1];
		NWrapperSingleton::getInstance().playerLeft(userID);
		break;

	case 'j':
		currentPlayer.id = (unsigned char)nEvent.data->Message.rawMsg[1];
		currentPlayer.name = &nEvent.data->Message.rawMsg[2];
		NWrapperSingleton::getInstance().playerJoined(currentPlayer);
		break;
	case '?':
		id_question = (unsigned char)nEvent.data->Message.rawMsg[1]; //My player ID that the host just have given me.
		NWrapperSingleton::getInstance().getMyPlayer().id = id_question;
		sendMyNameToHost();

		break;
	case 't':
	{
		States::ID state = nEvent.data->Message.rawMsg[1];
		NWrapperSingleton::getInstance().setSeed(seed);
		EventDispatcher::Instance().emit(NetworkSwitchState());
	}
	break;
	case 'w':
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
	case 's': // Serialized data, remove first character and send the rest to be deserialized
		dataString = std::string(nEvent.data->Message.rawMsg, nEvent.data->Message.sizeOfMsg);
		dataString.erase(0, 1); // remove the s

		// Send the serialized stringData as an event to the networkSystem which parses it.
		EventDispatcher::Instance().emit(NetworkSerializedPackageEvent(dataString));
		break;

	case 'z':
		EventDispatcher::Instance().emit(NetworkBackToLobby());
		break;
	case 'i': 
		EventDispatcher::Instance().emit(SettingsUpdatedEvent(std::string(nEvent.data->Message.rawMsg).substr(1,std::string::npos)));
		break;
	default:
		break;
	}
}

void NWrapperClient::sendMyNameToHost() {
	// Save the ID which the host has blessed us with
	std::string message = "?";
	message += NWrapperSingleton::getInstance().getMyPlayerName().c_str();
	sendMsg(message);
}

void NWrapperClient::updatePlayerList(std::list<Player>& playerList) {
	if (playerList.size() >= 2) {
		NWrapperSingleton::getInstance().resetPlayerList();

		for (auto currentPlayer : playerList) {
			NWrapperSingleton::getInstance().playerJoined(currentPlayer);
		}
	}
}