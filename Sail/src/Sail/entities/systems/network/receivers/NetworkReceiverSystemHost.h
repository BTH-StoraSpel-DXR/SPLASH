#pragma once
#include "NetworkReceiverSystem.h"


class NetworkReceiverSystemHost : public NetworkReceiverSystem {
public:
	NetworkReceiverSystemHost();
	virtual ~NetworkReceiverSystemHost();

	void handleIncomingData(const std::string& data) override;
	virtual void stop() override;

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
#endif

private:
	void endGame()                          override; // Start end timer for host
	void endMatchAfterTimer(const float dt) override; // Made for the host to quit the game after a set time
	void mergeHostsStats()                  override; // Host adds its data to global statistics before waiting for clients
	void prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) override;

	bool onEvent(const Event& event) override;

private:
	bool m_startEndGameTimer = false;
	bool m_finalKillCamOver = true;
};