#include "pch.h"
#include "NWrapper.h"
#include "Network/NetworkModule.hpp"
#include "Network/NWrapperSingleton.h"


NWrapper::NWrapper(Network* pNetwork) {
	this->initialize(pNetwork);
}

NWrapper::~NWrapper() {
}

void NWrapper::initialize(Network* pNetwork) {
	m_network = pNetwork;
	m_app = Application::getInstance();
}

void NWrapper::handleNetworkEvents(NetworkEvent nEvent) {
	switch (nEvent.eventType) {
	case NETWORK_EVENT_TYPE::NETWORK_ERROR:
		break;
	case NETWORK_EVENT_TYPE::CONNECTION_ESTABLISHED:
		playerJoined(nEvent.from_tcp_id);
		break;
	case NETWORK_EVENT_TYPE::CONNECTION_CLOSED:
		playerDisconnected(nEvent.from_tcp_id);
		break;
	case NETWORK_EVENT_TYPE::CONNECTION_RE_ESTABLISHED:
		playerReconnected(nEvent.from_tcp_id);
		break;
	case NETWORK_EVENT_TYPE::MSG_RECEIVED:
		decodeMessage(nEvent);
		break;
	default:
		break;
	}
}

TCP_CONNECTION_ID NWrapper::parseID(std::string& data) {
	if (data.size() > 63) {
		return 0;
	}
	if (data.size() < 1) {
		return 0;
	}
	else {
		// Remove opening ':' / '?' marker.
		data.erase(0, 1);

		std::string id_string = "";
		int lastIndex;
		for (lastIndex = 0; lastIndex < data.size(); lastIndex++) {
			if (data[lastIndex] == '\0' || data[lastIndex] == ':') {
				break;
			}
			else {
				id_string += data[lastIndex];
			}
		}

		data.erase(0, lastIndex);
		if (id_string != "") {
			return stoi(id_string);
		}
		else {
			return 0;
		}

	}
}

std::string NWrapper::parseName(std::string& data) {
	if (data.size() < 1) {
		return data;
	}
	else {
		// Remove first ':' marker
		data.erase(0, 1);

		int lastIndex;
		std::string parsedName = "";
		for (lastIndex = 0; lastIndex < MAX_PACKAGE_SIZE; lastIndex++) {
			if (data[lastIndex] == ':') { // Does parseID also remove the last ':'? no?
				break;
			}
			else {
				parsedName += data[lastIndex];
			}
		}

		data.erase(0, lastIndex);
		return parsedName;
	}
}

Message NWrapper::processChatMessage(const char* data) {
	Netcode::PlayerID id_m = data[0];
	std::string msg = &data[1];

	return Message{
		id_m,
		msg
	};
}

void NWrapper::updateStateLoadStatus(States::ID state, char status) {
	Player* myPlayer = NWrapperSingleton::getInstance().getPlayer(NWrapperSingleton::getInstance().getMyPlayerID());
	myPlayer->lastStateStatus.state = state;
	myPlayer->lastStateStatus.status = status;

	char msg[] = { ML_UPDATE_STATE_LOAD_STATUS, NWrapperSingleton::getInstance().getMyPlayerID(), state, status, ML_NULL};
	
	if (NWrapperSingleton::getInstance().isHost()) {
		sendMsgAllClients(msg, sizeof(msg));
	} else {
		sendMsg(msg, sizeof(msg));
	}
}

void NWrapper::sendMsg(const char* msg, size_t size, TCP_CONNECTION_ID tcp_id) {
	m_network->send(msg, size, tcp_id);
}

void NWrapper::sendMsgAllClients(const char* msg, size_t size) {
	m_network->send(msg, size, -1);
}


void NWrapper::sendSerializedDataAllClients(std::string data) {
	std::string msg;
	msg += ML_SERIALIZED;
	msg += data;	
	m_network->send(msg.c_str(), msg.length(), -1);
}

void NWrapper::sendSerializedDataToHost(std::string data) {
	std::string msg;
	msg += ML_SERIALIZED;
	msg += data;
	m_network->send(msg.c_str(), msg.length());
}