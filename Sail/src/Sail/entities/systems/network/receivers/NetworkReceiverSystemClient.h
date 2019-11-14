#pragma once
#include "NetworkReceiverSystem.h"


class NetworkReceiverSystemClient : public NetworkReceiverSystem {
public:
	NetworkReceiverSystemClient();
	virtual ~NetworkReceiverSystemClient();

	void handleIncomingData(const std::string& data) override;

private:
	void endMatch()                         override; // Start end timer for host
	void endMatchAfterTimer(const float dt) override; // Made for the host to quit the game after a set time
	void mergeHostsStats()                  override; // Host adds its data to global statistics before waiting for clients
	void prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) override;
};