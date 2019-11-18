#pragma once
#include "Sail/events/Event.h"
#include "Sail/netcode/NetcodeTypes.h"

struct StartThrowingEvent : public Event {
	StartThrowingEvent(const Netcode::ComponentID _netCompID)
		: Event(Event::Type::START_THROWING)
		, netCompID(_netCompID) {}
	const Netcode::ComponentID netCompID;
};
