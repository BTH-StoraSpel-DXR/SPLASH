#pragma once
#include <list>
#include "Sail/events/Event.h"
#include "Sail/states/StateIdentifiers.h"

struct NetworkSwitchState : public Event {
	NetworkSwitchState(const States::ID _state)
		: Event(Event::Type::NETWORK_SWITCH_STATE)
		, state(_state) {
	}
	~NetworkSwitchState() = default;

	States::ID state;
};