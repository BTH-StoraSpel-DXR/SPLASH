#include "pch.h"
#include "Network/NetworkModule.hpp"
#include "NetworkWrapper.h"

#include "../../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "../../SPLASH/src/game/events/NetworkDisconnectEvent.h"
#include "../../SPLASH/src/game/events/NetworkChatEvent.h"
#include "../../SPLASH/src/game/events/NetworkWelcomeEvent.h"
#include "../../SPLASH/src/game/states/LobbyState.h"

void NetworkWrapper::initialize() {
	m_network = new Network();
}

NetworkWrapper::~NetworkWrapper() {
	m_network->shutdown();
	delete m_network;
}

bool NetworkWrapper::host(int port) {
	if (!m_network->isServer())
	{
		//return m_network->setupHost(port);
	}
	return m_network->setupHost(port);
}

bool NetworkWrapper::connectToIP(char* adress) {

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

	return m_network->setupClient(IP, port);
}

void NetworkWrapper::sendMsg(std::string msg) {
	m_network->send(msg.c_str(), msg.length());
}

void NetworkWrapper::sendChatMsg(std::string msg) {
	
	
	if (m_network->isServer())
	{
		msg = std::string("mHost: ") + msg;
		m_network->send(msg.c_str(), msg.length(), -1);
		msg.erase(0, 1);
		msg = msg + std::string("\n");
		printf(msg.c_str());
	}
	else
	{
		msg = std::string("m") + msg;
		m_network->send(msg.c_str(), msg.length());
	}
}

void NetworkWrapper::sendMsgAllClients(std::string msg) {
	m_network->send(msg.c_str(), msg.length(), -1);
}

void NetworkWrapper::sendChatAllClients(std::string msg) {
	msg = std::string("m") + msg;
	m_network->send(msg.c_str(), msg.length(), -1);
}

void NetworkWrapper::checkForPackages() {
	m_network->checkForPackages(*this);
}

bool NetworkWrapper::isInitialized() {
	return m_network->isInitialized();
}

bool NetworkWrapper::isHost() {
	return m_network->isServer();
}

void NetworkWrapper::decodeMessage(NetworkEvent nEvent) {

	// These will be assigned in the switch case.
	unsigned int userID;
	std::string message;
	char charAsInt[4] = { 0 };
	std::list<player> playerList;	// Only used in 'w'-case but needs to be initialized
	player currentPlayer{ -1, "" };	// up here
	int charCounter = 0;			//

	switch (nEvent.data->msg[0])
	{
	case 'm':
		// handle chat message.

		// The host sends this message to all clients.
		if (m_network->isServer())
		{
			std::string tempMessage = std::string(nEvent.data->msg);
			tempMessage.erase(0, 1);
			message = std::string("m") + std::to_string(nEvent.clientID) + 
				std::string(": ") + tempMessage;

			sendMsgAllClients(message);

			// Print to screen
			message = message + std::string("\n");
			message.erase(0, 1);
			printf(message.c_str());
		}
		else
		{
			// Print to screen
			message = nEvent.data->msg + std::string("\n");
			message.erase(0, 1);
			printf(message.c_str());
		}

		Application::getInstance()->dispatchEvent(NetworkChatEvent(message));

		break;

	case 'd':
		// Only clients will get this message. Host handles this in playerDisconnected()
		
		// Get the user ID from the event data.
		for (int i = 0; i < 4; i++) {
			charAsInt[i] = nEvent.data->msg[1 + i];
		}
		userID = reinterpret_cast<int&>(charAsInt);

		// TODO:
		// Remove the user with this ID from the lobby and print out that it disconnected.
		printf((std::to_string(userID) + " disconnected. \n").c_str());
		Application::getInstance()->dispatchEvent(NetworkDisconnectEvent(userID));
		break;

	case 'j':
		// Only clients will get this message. Host handles this in playerJoined()

		// Get the user ID from the event data.
		for (int i = 0; i < 4; i++) {
			charAsInt[i] = nEvent.data->msg[1 + i];
		}
		userID = reinterpret_cast<int&>(charAsInt);

		// TODO:
		// Add this user id to the list of players in the lobby.
		// Print out that this ID joined the lobby.
		printf((std::to_string(userID) + " joined. \n").c_str());
		Application::getInstance()->dispatchEvent(NetworkJoinedEvent(player{ userID, "who?" }));
		break;

	case 'w':

		// Parse the welcome-package. Hosts should never recieve welcome packages
		// Parse first player
		charCounter = 1; // Content starts after the 'w'
		for (; true; charCounter++)
		{
			if (nEvent.data->msg[charCounter] == ':') {
				playerList.push_back(currentPlayer);
				currentPlayer.name = "";
				charCounter++;
				break;	// First name has been found
			}
			else {
				currentPlayer.name += nEvent.data->msg[charCounter];
			}
		}

		for (; charCounter < MAX_PACKAGE_SIZE; charCounter++)
		{
			// If delimiter, we just finished with a player name
			if (nEvent.data->msg[charCounter] == ':') {
				playerList.push_back(currentPlayer);
				currentPlayer.name = "";
			}
			// If nullterminator, the msg is over.
			else if (nEvent.data->msg[charCounter] == '\0') {
				break;
			}
			// If neither above, add char to currentplayer
			else {
				currentPlayer.name += nEvent.data->msg[charCounter];
			}
		}

		Application::getInstance()->dispatchEvent(NetworkWelcomeEvent(playerList));
		break;

	default:
		printf((std::string("Error: Packet message with key: ") + 
			nEvent.data->msg[0] + "can't be handled. \n").c_str());
		break;
	}

}

void NetworkWrapper::playerDisconnected(ConnectionID id) {
	/*
		Send disconnect message to all clients if host.
	*/
	if (m_network->isServer())
	{
		char msg[64];
		int intid = (int)id;
		char* int_asChar = reinterpret_cast<char*>(&intid);

		msg[0] = 'd';
		for (int i = 0; i < 4; i++) {
			msg[i + 1] = int_asChar[i];
		}

		// Send to all clients that soneone disconnected and which id.
		m_network->send(msg, sizeof(msg), -1);

		printf((std::to_string(intid) + " disconnected. \n").c_str());

		// Send id to menu / game state
		Application::getInstance()->dispatchEvent(NetworkDisconnectEvent(intid));
	}
	else
	{
		printf("Host disconnected. \n");
	}
}

void NetworkWrapper::playerReconnected(ConnectionID id) {
	/*
		This remains unimplemented.
	*/
}

void NetworkWrapper::playerJoined(ConnectionID id) {
	if (m_network->isServer())
	{
		char msg[64] = { 0 };
		unsigned int intid = id;
		char* int_asChar = reinterpret_cast<char*>(&intid);

		msg[0] = 'j';
		for (int i = 0; i < 4; i++) {
			msg[i + 1] = int_asChar[i];
		}

		// Send to all clients that someone joined and which id.
		m_network->send(msg, sizeof(msg), -1);

		// Add this user id to the list of players in the lobby.
		// Print out that this ID joined the lobby.
		printf((std::to_string(intid) + " joined. \n").c_str());

		Application::getInstance()->dispatchEvent(NetworkJoinedEvent(player{ intid, "" }));
	}
}

void NetworkWrapper::handleNetworkEvents(NetworkEvent nEvent) {
	switch (nEvent.eventType)
	{
	case NETWORK_EVENT_TYPE::NETWORK_ERROR:
		break;
	case NETWORK_EVENT_TYPE::CLIENT_JOINED:
		playerJoined(nEvent.clientID);
		break;
	case NETWORK_EVENT_TYPE::CLIENT_DISCONNECTED:
		playerDisconnected(nEvent.clientID);
		break;
	case NETWORK_EVENT_TYPE::CLIENT_RECONNECTED:
		playerReconnected(nEvent.clientID);
		break;
	case NETWORK_EVENT_TYPE::MSG_RECEIVED:
		decodeMessage(nEvent);
		break;
	default:
		break;
	}
}
