#pragma once

#include "Sail/events/Event.h"
#include "Network/NWrapper.h"

struct NetworkWelcomeEvent : public Event {
	NetworkWelcomeEvent(const Player _player)
		: Event(Event::Type::NETWORK_WELCOME)
		, player(_player) {
	}
	~NetworkWelcomeEvent() = default;

	const Player player;
};