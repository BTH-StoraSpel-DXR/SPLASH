#pragma once

#include "Sail/events/Event.h"
#include "Sail/states/StateIdentifiers.h"
#include "Sail/netcode/NetworkedStructs.h"

/*
	This event is sent when a player reports its current status within a state.

	Example:
		1) When all players enters gameState they start by reporting their status for gameState as 0 in NWrapper->updateStateLoadStatus(States::ID state, char status) to indicate that they
		are in gameState but not yet ready to start.
		2) When the players have loaded all resources they report status 1 indicating that they are ready.
		3) When all players are ready the game can start.
*/

struct NetworkUpdateStateLoadStatus : public Event {
	NetworkUpdateStateLoadStatus(States::ID state, Netcode::PlayerID player, char status) : Event(Event::Type::NETWORK_UPDATE_STATE_LOAD_STATUS), stateID(state), playerID(player), status(status) {}
	~NetworkUpdateStateLoadStatus() = default;

	States::ID stateID;
	Netcode::PlayerID playerID;
	char status;
};