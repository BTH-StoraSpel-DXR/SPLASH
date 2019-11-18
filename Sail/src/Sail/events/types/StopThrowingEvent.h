#pragma once
#include "Sail/events/Event.h"
#include "Sail/netcode/NetcodeTypes.h"

struct StopThrowingEvent : public Event {
	StopThrowingEvent(const Netcode::ComponentID _netCompID)
		: Event(Event::Type::STOP_THROWING)
		, netCompID(_netCompID) {}
	const Netcode::ComponentID netCompID;
};