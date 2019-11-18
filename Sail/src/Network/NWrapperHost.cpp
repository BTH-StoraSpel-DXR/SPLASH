#include "pch.h"
#include "NWrapperHost.h"
#include "Network/NetworkModule.hpp"

#include "Sail/events/EventDispatcher.h"
#include "../../SPLASH/src/game/states/LobbyState.h"
#include "NWrapperSingleton.h"
#include "../../SPLASH/src/game/events/NetworkChatEvent.h"
#include "../../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "../../SPLASH/src/game/events/NetworkSerializedPackageEvent.h"
#include "Sail/events/types/NetworkUpdateStateLoadStatus.h"
#include "Sail/events/types/NetworkPlayerRequestedTeamChange.h"
#include "Sail/events/types/NetworkPlayerChangedTeam.h"

#include "Sail/events/types/NetworkUpdateStateLoadStatus.h"


bool NWrapperHost::host(int port) {
	bool result = m_network->host(port);
	return result;
}

bool NWrapperHost::connectToIP(char*) {
	// Do nothing. clients connect to hosts, not the other way around.
	return false;
}

void NWrapperHost::setLobbyName(std::string name) {
	m_lobbyName = name;
	updateServerDescription();
}

void NWrapperHost::sendChatMsg(std::string msg) {
	std::string data;
	data += ML_CHAT;
	data += NWrapperSingleton::getInstance().getMyPlayer().id;
	data += msg;
	m_network->send(data.c_str(), data.length() + 1, -1);
}

void NWrapperHost::updateServerDescription() {
	m_serverDescription = m_lobbyName + " (" + std::to_string(NWrapperSingleton::getInstance().getPlayers().size()) + "/12)";

	m_network->setServerMetaDescription(m_serverDescription.c_str(), m_serverDescription.length() + 1);
}

void NWrapperHost::sendSerializedDataToClient(std::string data, Netcode::PlayerID PlayeriD) {
	std::string msg;
	msg += ML_SERIALIZED;
	msg += data;

	for (auto p : m_connectionsMap) {
		if (p.second == PlayeriD) {
			m_network->send(msg.c_str(), msg.length() + 1, p.first);
			break;
		}
	}
}

#ifdef DEVELOPMENT

const std::map<TCP_CONNECTION_ID, unsigned char>& NWrapperHost::getConnectionMap() {
	return m_connectionsMap;
}

const std::string& NWrapperHost::getServerDescription() {
	return m_serverDescription;
}

const std::string& NWrapperHost::getLobbyName() {
	return m_lobbyName;
}

#endif // DEVELOPMENT

void NWrapperHost::playerJoined(TCP_CONNECTION_ID tcp_id) {
	// Generate an ID for the client that joined and send that information.

	m_IdDistribution++;
	unsigned char newPlayerId = m_IdDistribution;

	if (NWrapperSingleton::getInstance().playerJoined(Player{ newPlayerId, "NoName" }, false)) {
		m_connectionsMap.insert(std::pair<TCP_CONNECTION_ID, unsigned char>(tcp_id, newPlayerId));
		//Send the newPlayerId to the new player and request a name, which upon retrieval will be sent to all clients.
		char msg[3] = {ML_NAME_REQUEST, newPlayerId, ML_NULL};
		m_network->send(msg, sizeof(msg), tcp_id);
	} else {
		m_IdDistribution--;
	}
	
	updateServerDescription();
}

void NWrapperHost::playerDisconnected(TCP_CONNECTION_ID tcp_id) {
	Netcode::PlayerID playerID = m_connectionsMap.at(tcp_id);
	PlayerLeftReason reason = m_network->wasKicked(tcp_id) ? PlayerLeftReason::KICKED : PlayerLeftReason::CONNECTION_LOST;
	
	char msg[] = { ML_DISCONNECT, playerID, (char)reason, ML_NULL};

	// Send to all clients that someone disconnected and which id.
	m_network->send(msg, sizeof(msg), -1);

	// Send id to menu / game state
	NWrapperSingleton::getInstance().playerLeft(playerID, true, reason);
	updateServerDescription();
}

void NWrapperHost::playerReconnected(TCP_CONNECTION_ID id) {
	// Nothing implemented so far.
}

void NWrapperHost::decodeMessage(NetworkEvent nEvent) {
	// These will be assigned in the switch case.
	std::string message;
	char charAsInt[4] = { 0 };
	std::list<Player> playerList;	// Only used in 'w'-case but needs to be initialized up here
	int charCounter = 0;			//
	std::string id_string = "";				//
	std::string remnants = "";				//
	unsigned int id_number = 0;			//
	std::string id = "";			// used in 'm'
	std::string remnants_m = "";
	Message processedMessage;
	std::string dataString;
	std::string name;

	switch (nEvent.data->Message.rawMsg[0])
	{
	case ML_CHAT:
		// Send out the already formatted message to clients so that they can process the message.
		sendMsgAllClients(nEvent.data->Message.rawMsg, nEvent.data->Message.sizeOfMsg);

		// Process the chat message
		processedMessage = processChatMessage(&nEvent.data->Message.rawMsg[1]);

		// Dispatch to lobby
		EventDispatcher::Instance().emit(NetworkChatEvent(processedMessage));

		break;

	case ML_DISCONNECT:
		// Only clients will get this message. Host handles this in playerDisconnected()
		break;

	case ML_JOIN:
		// Only clients will get this message. Host handles this in playerJoined()
		break;

	case ML_NAME_REQUEST:

		name = &nEvent.data->Message.rawMsg[1];
		updateClientName(nEvent.from_tcp_id, m_connectionsMap[nEvent.from_tcp_id], name);

		break;
	case ML_UPDATE_STATE_LOAD_STATUS:
		{		
			sendMsgAllClients(nEvent.data->Message.rawMsg, nEvent.data->Message.sizeOfMsg);
			
			Netcode::PlayerID playerID = (Netcode::PlayerID)nEvent.data->Message.rawMsg[1];
			States::ID state = (States::ID)nEvent.data->Message.rawMsg[2];
			char status = nEvent.data->Message.rawMsg[3];

			Player* player = NWrapperSingleton::getInstance().getPlayer(playerID);
			player->lastStateStatus.state = state;
			player->lastStateStatus.status = status;
			EventDispatcher::Instance().emit(NetworkUpdateStateLoadStatus(state, playerID, status));
		
		}

		
		break;
	case ML_WELCOME:
		// Only clients receive welcome packages.
		break;
	case ML_SERIALIZED: // Serialized data, remove first character and send the rest to be deserialized
		dataString = std::string(nEvent.data->Message.rawMsg, nEvent.data->Message.sizeOfMsg);
		dataString.erase(0, 1); // remove the s

		// Send the serialized stringData as an event to the networkSystem which parses it.
		EventDispatcher::Instance().emit(NetworkSerializedPackageEvent(dataString));
		break;
	case ML_TEAM_REQUEST:
	{
		char team = nEvent.data->Message.rawMsg[1];
		Netcode::PlayerID playerID = m_connectionsMap[nEvent.from_tcp_id];
		EventDispatcher::Instance().emit(NetworkPlayerRequestedTeamChange(playerID, team));
	}
	default:
		break;
	}
}

void NWrapperHost::updateClientName(TCP_CONNECTION_ID tcp_id, Netcode::PlayerID playerId, std::string& name) {
	
	Player* p = NWrapperSingleton::getInstance().getPlayer(playerId);
	p->name = name;

	// Send a welcome package to the new Player, letting them know who's in the party
	for (auto currentPlayer : NWrapperSingleton::getInstance().getPlayers()) {
		std::string joinedPackage;
		joinedPackage += ML_JOIN;
		joinedPackage += currentPlayer.id; //This will break if playerId == 0
		joinedPackage += currentPlayer.name;

		sendMsg(joinedPackage.c_str(), joinedPackage.length() + 1, tcp_id);

	}

	for (auto p : NWrapperSingleton::getInstance().getPlayers()) {
		char msg[] = { ML_UPDATE_STATE_LOAD_STATUS, p.id, p.lastStateStatus.state, p.lastStateStatus.status, ML_NULL };
		sendMsg(msg, sizeof(msg), tcp_id);

		char msgTeam[] = { ML_TEAM_REQUEST, NWrapperSingleton::getInstance().getPlayer(p.id)->team, p.id, ML_NULL };
		sendMsg(msgTeam, sizeof(msgTeam), tcp_id);

	}

	if (p->justJoined) {
		p->justJoined = false;

#ifdef DEVELOPMENT
		if (playerId == 0) {
			SAIL_LOG_ERROR("Critical Error: Playerid equal to 0 and will be interpreted as a null terminator when appended to a string.");
		}
#endif // DEVELOPMENT

		std::string joinedPackage;
		joinedPackage += ML_JOIN;
		joinedPackage += playerId; //This will break if playerId == 0
		joinedPackage += name;


		// Send a PlayerJoined message to all other players
		for (auto player : m_connectionsMap) {
			if (player.first != tcp_id) {
				sendMsg(joinedPackage.c_str(), joinedPackage.length() + 1, player.first);
			}
		}

		EventDispatcher::Instance().emit(NetworkJoinedEvent(*p));
	} else {
		//TODO: Should the players be able to change name during a match?
		//If they should, the host network related code to inform other players can be implemented here.
	}
}

void NWrapperHost::setClientState(States::ID state, Netcode::PlayerID id) {
	char msg[] = {ML_CHANGE_STATE, (char)state, ML_NULL};
	
	if (id == 255) {
		sendMsgAllClients(msg, sizeof(msg));
	} else {
		for (auto p : m_connectionsMap) {
			if (p.second == id) {
				sendMsg(msg, sizeof(msg), p.first);
				break;
			}
		}
	}
}

void NWrapperHost::kickPlayer(Netcode::PlayerID playerId) {
	if (playerId != 0) {
		for (auto p : m_connectionsMap) {
			if (p.second == playerId) {
				m_network->kickConnection(p.first);
			}
		}
	} else {
		SAIL_LOG("You Cant Kick The Host");
	}
	
}

void NWrapperHost::updateGameSettings(std::string s) {
	std::string msg;
	msg += ML_UPDATE_SETTINGS;
	msg += s;
	sendMsgAllClients(msg.c_str(), msg.length() + 1);
}

void NWrapperHost::requestTeam(char team) {
	setTeamOfPlayer(team, NWrapperSingleton::getInstance().getMyPlayerID());
}

void NWrapperHost::setTeamOfPlayer(char team, Netcode::PlayerID playerID, bool dispatch) {
	if (team == NWrapperSingleton::getInstance().getPlayer(playerID)->team) {
		return;
	} else {
		char msg[] = {ML_TEAM_REQUEST, team, playerID, (char)dispatch, ML_NULL};

		sendMsgAllClients(msg, sizeof(msg));
		NWrapperSingleton::getInstance().getPlayer(playerID)->team = team;
		if (dispatch) {
			EventDispatcher::Instance().emit(NetworkPlayerChangedTeam(playerID));
		}
	}
}

