#pragma once
#include "Sail/events/Event.h"
#include "Sail/netcode/NetcodeTypes.h"

struct LoopShootingEvent : public Event {
	LoopShootingEvent(const Netcode::ComponentID _netCompID, float lowPassFrequency)
		: Event(Event::Type::LOOP_SHOOTING)
		, netCompID(_netCompID)
		, lowPassFrequency(lowPassFrequency)
	{}
	const Netcode::ComponentID netCompID;
	float lowPassFrequency;
};