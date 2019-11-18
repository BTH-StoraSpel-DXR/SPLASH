#pragma once

#include "Sail/events/Event.h"
#include "Sail/states/StateIdentifiers.h"

struct NetworkChangeStateEvent : public Event {

	NetworkChangeStateEvent(States::ID state) : Event(Event::Type::NETWORK_CHANGE_STATE), stateID(state) { }
	~NetworkChangeStateEvent() = default;

	States::ID stateID;
};