#pragma once

#include "Sail/events/Event.h"
#include "Sail/netcode/NetworkedStructs.h"

struct NetworkTeamColorRequest : public Event {
	NetworkTeamColorRequest(Netcode::PlayerID playerID, char team, char teamColorID) : Event(Event::Type::NETWORK_TEAM_REQUESTED_COLOR_CHANGE), playerID(playerID), team(team), teamColorID(teamColorID) {}
	~NetworkTeamColorRequest() = default;

	Netcode::PlayerID playerID;
	char team;
	char teamColorID;
};