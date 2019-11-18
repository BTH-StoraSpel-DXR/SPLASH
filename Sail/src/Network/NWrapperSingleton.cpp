#include "pch.h"
#include "NWrapperSingleton.h"
#include "Network/NetworkModule.hpp"
#include "Network/NetworkStructs.hpp"
#include "Sail.h"
#include "../../SPLASH/src/game/events/NetworkLanHostFoundEvent.h"
#include "Sail/entities/systems/network/NetworkSenderSystem.h"
#include "Sail/events/EventDispatcher.h"

#include "../../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "../../SPLASH/src/game/events/NetworkDisconnectEvent.h"
#include "../../SPLASH/src/game/events/NetworkWelcomeEvent.h"
#include "../../SPLASH/src/game/events/NetworkNameEvent.h"

NWrapperSingleton::~NWrapperSingleton() {
	if (m_isInitialized && m_wrapper != nullptr) {
		delete m_wrapper;
		
	}

	Memory::SafeDelete(m_network);
}

NWrapperSingleton& NWrapperSingleton::getInstance() {
	static NWrapperSingleton instance;
	return instance;
}
NWrapperSingleton::NWrapperSingleton() {
	m_network = SAIL_NEW Network;

	m_network->initialize();

	m_playerLimit = 12;
}

bool NWrapperSingleton::host(int port) {
	this->initialize(true);

	if (m_isHost) {
		if (m_wrapper->host(port) == true) {
			return true;
		} else {
			resetWrapper();
		}
	}

	return false;
}

bool NWrapperSingleton::connectToIP(char* adress) {
	this->initialize(false);
	
	if (!m_isHost) {
		if (m_wrapper->connectToIP(adress) == true) {
			return true;
		} else {
			resetWrapper();
		}
	}
	
	return false;
}

bool NWrapperSingleton::isHost() {
	return m_isHost;
}

NWrapper* NWrapperSingleton::getNetworkWrapper() {
	return m_wrapper;
}

void NWrapperSingleton::searchForLobbies() {
	m_network->searchHostsOnLan();
}

void NWrapperSingleton::checkForPackages() {
	m_network->checkForPackages(*this);
}

void NWrapperSingleton::stopUDP() {
	m_network->stopUDP();
}

void NWrapperSingleton::startUDP(){
	m_network->startUDP();
}

void NWrapperSingleton::resetPlayerList() {
	m_players.clear();
}

bool NWrapperSingleton::playerJoined(const Player& player, bool dispatchEvent) {
	Player newPlayer(player.id, player.name.c_str());	// This will fix currupt string size.
	
	if (m_players.size() < m_playerLimit) {
		m_players.push_back(newPlayer);
		if (dispatchEvent) {
			EventDispatcher::Instance().emit(NetworkJoinedEvent(player));
		}
		return true;
	}

	return false;
}

bool NWrapperSingleton::playerLeft(Netcode::PlayerID& id, bool dispatchEvent, PlayerLeftReason reason) {
	// Linear search to get target 'player' struct, then erase that from the list
	Player* toBeRemoved = nullptr;
	int pos = 0;
	for (auto playerIt : m_players) {
		if (playerIt.id == id) {
			toBeRemoved = &playerIt;
			if (dispatchEvent) {
				EventDispatcher::Instance().emit(NetworkDisconnectEvent(playerIt, reason));
			}
			m_players.remove(*toBeRemoved);

			return true;
		}
	}

	SAIL_LOG_WARNING("PlayerLeft was called with a none existing playerID");

	return false;
}

Player& NWrapperSingleton::getMyPlayer() {
	return m_me;
}

Player* NWrapperSingleton::getPlayer(const Netcode::PlayerID id) {
	Player* foundPlayer = nullptr;
	for (Player& player : m_players) {
		if (player.id == id) {
			foundPlayer = &player;
			break;
		}
	}

	return foundPlayer;
}

const std::list<Player>& NWrapperSingleton::getPlayers() const {
	return m_players;
}


void NWrapperSingleton::setPlayerName(const char* name) {
	m_me.name = name;
}

void NWrapperSingleton::setPlayerID(const Netcode::PlayerID ID) {
	m_me.id = ID;
}

std::string& NWrapperSingleton::getMyPlayerName() {
	return m_me.name;
}

Netcode::PlayerID NWrapperSingleton::getMyPlayerID() {
	return m_me.id;
}

unsigned int NWrapperSingleton::getSeed() const {
	return m_seed;
}

void NWrapperSingleton::setSeed(char seed) {
	m_seed = static_cast<unsigned int>(seed);
}

void NWrapperSingleton::setNSS(NetworkSenderSystem* NSS_) {
	NSS = NSS_;
}

void NWrapperSingleton::queueGameStateNetworkSenderEvent(Netcode::MessageType type, Netcode::MessageData* data, bool alsoSendToSelf) {
	if (type < Netcode::MessageType::CREATE_NETWORKED_PLAYER || Netcode::MessageType::COUNT < type) {
		SAIL_LOG_ERROR("TRIED TO QUEUE AN INVALID EVENT: (" + std::to_string((int)type) + ")");
	}

	// Cleaning is handled by the NSS later on.
	NetworkSenderEvent* e = SAIL_NEW NetworkSenderEvent;
	e->type = type;

	e->data = data;
	e->alsoSendToSelf = alsoSendToSelf;

	NSS->queueEvent(e);
}


size_t NWrapperSingleton::averagePacketSizeSinceLastCheck() {
	size_t average = 0;
	if (m_network) {
		average = m_network->averagePacketSizeSinceLastCheck();
	}
	return average;
}

void NWrapperSingleton::initialize(bool asHost) {
	if (m_isInitialized == false) {
		m_isInitialized = true;

		if (asHost) {
			m_isHost = true;
			m_wrapper = SAIL_NEW NWrapperHost(m_network);
		}
		else {
			m_isHost = false;
			m_wrapper = SAIL_NEW NWrapperClient(m_network);
		}
	}
}

void NWrapperSingleton::resetWrapper() {
	resetPlayerList();
	m_isInitialized = false;
	m_isHost = false;
	delete this->m_wrapper;
	this->m_wrapper = nullptr;
}

void NWrapperSingleton::resetNetwork() {
	m_network->shutdown();
	m_network->initialize();
}

void NWrapperSingleton::handleNetworkEvents(NetworkEvent nEvent) {
	if (nEvent.eventType == NETWORK_EVENT_TYPE::HOST_ON_LAN_FOUND) {
		NetworkLanHostFoundEvent event0(
			nEvent.data->HostFoundOnLanData.ip_full,
			nEvent.data->HostFoundOnLanData.hostPort,
			nEvent.data->HostFoundOnLanData.description
		);
		
		EventDispatcher::Instance().emit(event0);
	}

	if (m_wrapper) {
		m_wrapper->handleNetworkEvents(nEvent);
	}
}