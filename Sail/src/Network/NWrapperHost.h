#pragma once

#include "NWrapper.h"
#include <string>

class NWrapperHost : public NWrapper {
public:
	NWrapperHost(Network* pNetwork) : NWrapper(pNetwork) {}
	virtual ~NWrapperHost() {}

	bool host(int port = 54000);
	bool connectToIP(char* = "127.0.0.1:54000");
	void setLobbyName(std::string name);
	void updateServerDescription();

private:
	std::map<TCP_CONNECTION_ID, Netcode::PlayerID> m_connectionsMap;
	unsigned char m_IdDistribution = 0;
	std::string m_lobbyName = "";
	std::string m_serverDescription = "";

	void sendChatMsg(std::string msg);
 
	void playerJoined(TCP_CONNECTION_ID tcp_id);
	void playerDisconnected(TCP_CONNECTION_ID tcp_id);
	void playerReconnected(TCP_CONNECTION_ID tcp_id);
	void decodeMessage(NetworkEvent nEvent);
	void updateClientName(TCP_CONNECTION_ID tcp_id, Netcode::PlayerID playerId, std::string& name);

	virtual void switchToState(States::ID state);
};