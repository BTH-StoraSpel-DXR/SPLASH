#pragma once

#include "../../../../Sail/src/Sail/events/Event.h"

class NetworkTestEvent : public Event {
public:
	NetworkTestEvent(std::string message)
		: Event(Event::NETWORK_TEST)
		, m_message(message)
	{

	}
	~NetworkTestEvent() {}

	inline std::string getMessage() const { return m_message; };

	static Type getStaticType() {
		return Event::NETWORK_TEST;
	}

private:
	std::string m_message;
};