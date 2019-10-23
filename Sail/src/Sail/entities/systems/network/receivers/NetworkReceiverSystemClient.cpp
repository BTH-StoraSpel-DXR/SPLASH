#include "pch.h"
#include "NetworkReceiverSystemClient.h"

NetworkReceiverSystemClient::NetworkReceiverSystemClient() {

}

NetworkReceiverSystemClient::~NetworkReceiverSystemClient() {

}

void NetworkReceiverSystemClient::handleIncomingData(std::string data) {
	pushDataToBuffer(data);
}
