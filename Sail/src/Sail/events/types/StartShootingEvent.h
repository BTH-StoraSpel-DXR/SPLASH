#pragma once
#include "Sail/events/Event.h"
#include "Sail/netcode/NetcodeTypes.h"

struct StartShootingEvent : public Event {
	StartShootingEvent(const Netcode::ComponentID _netCompID, float lowPassFrequency)
		: Event(Event::Type::START_SHOOTING)
		, netCompID(_netCompID)
		, lowPassFrequency(lowPassFrequency)
	{}
	const Netcode::ComponentID netCompID;
	float lowPassFrequency;
};
