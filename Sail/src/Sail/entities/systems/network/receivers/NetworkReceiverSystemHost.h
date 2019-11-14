#pragma once
#include "NetworkReceiverSystem.h"


class NetworkReceiverSystemHost : public NetworkReceiverSystem {
public:
	NetworkReceiverSystemHost();
	virtual ~NetworkReceiverSystemHost();

	void handleIncomingData(const std::string& data) override;

private:
	void endMatch()                         override; // Start end timer for host
	void endMatchAfterTimer(const float dt) override; // Made for the host to quit the game after a set time
	void mergeHostsStats()                  override; // Host adds its data to global statistics before waiting for clients
	void prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) override;


private:
	bool m_startEndGameTimer = false;
};