#pragma once


#include "NetworkReceiverSystem.h"


class NetworkReceiverSystemHost : public NetworkReceiverSystem {
public:
	NetworkReceiverSystemHost();
	~NetworkReceiverSystemHost();


	// Push incoming data strings to the back of a FIFO list
	void handleIncomingData(std::string data) override;
private:

};