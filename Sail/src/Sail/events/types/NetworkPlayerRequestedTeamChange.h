#pragma once

#include "Sail/events/Event.h"
#include "Sail/netcode/NetworkedStructs.h"

struct NetworkPlayerRequestedTeamChange : public Event {
	NetworkPlayerRequestedTeamChange(Netcode::PlayerID playerID, char team) : Event(Event::Type::NETWORK_PLAYER_REQUESTED_TEAM_CHANGE), playerID(playerID), team(team) {}
	~NetworkPlayerRequestedTeamChange() = default;

	Netcode::PlayerID playerID;
	char team;
};