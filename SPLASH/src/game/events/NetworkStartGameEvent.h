#pragma once

#include "../../../../Sail/src/Sail/events/Event.h"

class NetworkStartGameEvent : public Event {
public:
	NetworkStartGameEvent()
		: Event(Event::NETWORK_START_GAME)
	{}
	~NetworkStartGameEvent() {}

	
	static Type getStaticType() {
		return Event::NETWORK_START_GAME;
	}

private:
};