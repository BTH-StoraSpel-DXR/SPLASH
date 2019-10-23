#include "pch.h"
#include "NetworkReceiverSystemHost.h"
#include "../NetworkSenderSystem.h"

NetworkReceiverSystemHost::NetworkReceiverSystemHost() {

}

NetworkReceiverSystemHost::~NetworkReceiverSystemHost() {

}


void NetworkReceiverSystemHost::handleIncomingData(std::string data) {
	pushDataToBuffer(data);

	// The host will also save the data in the sender system so that it can be forwarded to all other clients
	m_netSendSysPtr->pushDataToBuffer(data);
}