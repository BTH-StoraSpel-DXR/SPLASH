#pragma once

#include "Sail/events/Event.h"
#include "Sail/netcode/NetworkedStructs.h"

struct NetworkPlayerChangedTeam : public Event {
	NetworkPlayerChangedTeam(Netcode::PlayerID playerID) : Event(Event::Type::NETWORK_PLAYER_CHANGED_TEAM), playerID(playerID) {}
	~NetworkPlayerChangedTeam() = default;

	Netcode::PlayerID playerID;
};