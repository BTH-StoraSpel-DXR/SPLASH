#pragma once


#include "NWrapperHost.h"
#include "NWrapperClient.h"

class NetworkSenderSystem;

struct NetworkSenderEvent {
	Netcode::MessageType type;
	Netcode::MessageData* data = nullptr;  

	// All events will by default be handled by NetworkReceiverSystem for
	// both the people receiving the event and the person sending it.
	bool alsoSendToSelf = true;

	virtual ~NetworkSenderEvent() {
		if (data) {
			delete data;
			data = nullptr;
		}
	}
};


class NWrapperSingleton : public NetworkEventHandler {
public:
	virtual ~NWrapperSingleton();

	// Initializes NetworkWrapper as NetworkWrapperHost
	bool host(int port = 54000);
	// Initializes NetworkWrapper as NetworkWrapperClient
	bool connectToIP(char* = "127.0.0.1:54000");
	bool isHost();
	void resetWrapper();
	void resetNetwork();
	NWrapper* getNetworkWrapper();
	void searchForLobbies();
	void checkForPackages();
	void stopUDP();
	void startUDP();

	void resetPlayerList();
	bool playerJoined(const Player& player, bool dispatchEvent = true);
	bool playerLeft(Netcode::PlayerID& id, bool dispatchEvent = true);

	Player& getMyPlayer();
	Player* getPlayer(const Netcode::PlayerID id);

	const std::list<Player>& getPlayers() const;

	void setPlayerName(const char* name);
	void setPlayerID(const Netcode::PlayerID ID);
	std::string& getMyPlayerName();
	Netcode::PlayerID getMyPlayerID();
	unsigned getSeed() const;
	void setSeed(char seed);

	// Specifically for One-Time-Events during the gamestate
	void setNSS(NetworkSenderSystem* NSS);

	// Messages sent to self will be dealt with in NetworkReceiverSystem
	void queueGameStateNetworkSenderEvent(Netcode::MessageType type, Netcode::MessageData* messageData, bool alsoSendToSelf = true);

	size_t averagePacketSizeSinceLastCheck();
private:
	// Specifically for One-Time-Events during the gamestate
	NetworkSenderSystem* NSS = nullptr;
private:
	// Called by 'host' & 'connectToIP'
	void initialize(bool asHost);
	Network* m_network = nullptr;
	NWrapper* m_wrapper = nullptr;
	bool m_isInitialized = false;
	bool m_isHost = false;

	unsigned int m_playerLimit;
	unsigned int m_seed;

	Player m_me;
	std::list<Player> m_players;

	void handleNetworkEvents(NetworkEvent nEvent);

	// -+-+-+-+-+- Singleton requirements below -+-+-+-+-+-
public:
	NWrapperSingleton(NWrapperSingleton const&) = delete;
	void operator=(NWrapperSingleton const&) = delete;
	static NWrapperSingleton& getInstance();
private:
	NWrapperSingleton();
};