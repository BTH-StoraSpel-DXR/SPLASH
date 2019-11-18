#pragma once
#include "Sail/events/Event.h"
#include "Sail/netcode/NetcodeTypes.h"

struct StopShootingEvent : public Event {
	StopShootingEvent(const Netcode::ComponentID _netCompID, float lowPassFrequency)
		: Event(Event::Type::STOP_SHOOTING)
		, netCompID(_netCompID)
		, lowPassFrequency(lowPassFrequency)
	{}
	const Netcode::ComponentID netCompID;
	float lowPassFrequency;
};