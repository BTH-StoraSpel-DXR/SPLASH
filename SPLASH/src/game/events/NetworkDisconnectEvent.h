#pragma once

#include "Sail/events/Event.h"
#include "Network/NWrapper.h"

struct NetworkDisconnectEvent : public Event {
	NetworkDisconnectEvent(const Player _player, const PlayerLeftReason reason)
		: Event(Event::Type::NETWORK_DISCONNECT)
		, player(_player), reason(reason) { }
	~NetworkDisconnectEvent() = default;

	const Player player;
	const PlayerLeftReason reason;
};