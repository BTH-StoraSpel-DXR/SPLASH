#pragma once

#include "NetworkReceiverSystem.h"



class NetworkReceiverSystemClient : public NetworkReceiverSystem {
public:
	NetworkReceiverSystemClient();
	~NetworkReceiverSystemClient();

	// Push incoming data strings to the back of a FIFO list
	void handleIncomingData(std::string data) override;

private:

};