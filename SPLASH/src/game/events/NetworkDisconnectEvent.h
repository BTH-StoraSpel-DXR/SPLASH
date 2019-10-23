#pragma once

#include "../../../../Sail/src/Sail/events/Event.h"
#include "../states/LobbyState.h"

#include "Sail/netcode/NetcodeTypes.h"

class NetworkDisconnectEvent : public Event {
public:
	NetworkDisconnectEvent(Netcode::PlayerID ID) : Event(Event::NETWORK_DISCONNECT) {
		m_player_id = ID;
	}
	~NetworkDisconnectEvent() {}

	inline Netcode::PlayerID getPlayerID() const { return m_player_id; };

	static Type GetStaticType() {
		return Event::NETWORK_DISCONNECT;
	}

private:
	Netcode::PlayerID m_player_id;

};