#include "pch.h"
#include "NetworkReceiverSystemClient.h"

//#include "Network/NWrapperSingleton.h"
#include "../SPLASH/src/game/states/GameState.h"

NetworkReceiverSystemClient::NetworkReceiverSystemClient() 
{}

NetworkReceiverSystemClient::~NetworkReceiverSystemClient() 
{}


void NetworkReceiverSystemClient::handleIncomingData(const std::string& data) {
	pushDataToBuffer(data);
}

void NetworkReceiverSystemClient::endMatch() {
	m_gameStatePtr->requestStackClear();
	m_gameStatePtr->requestStackPush(States::EndGame);
}


// HOST ONLY FUNCTIONS
void NetworkReceiverSystemClient::endMatchAfterTimer(const float dt) {}
void NetworkReceiverSystemClient::mergeHostsStats() {}
void NetworkReceiverSystemClient::prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) {}
