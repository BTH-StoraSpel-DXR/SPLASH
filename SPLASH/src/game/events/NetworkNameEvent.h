#pragma once
#include <string>
#include "Sail/events/Event.h"
#include "Network/NetworkStructs.hpp"

struct NetworkNameEvent : public Event {
	NetworkNameEvent(const std::string& name, const TCP_CONNECTION_ID id)
		: Event(Event::Type::NETWORK_NAME)
		, repliedName(name)
		, idOfSender(id) { }
	~NetworkNameEvent() = default;

	const std::string repliedName;
	const TCP_CONNECTION_ID idOfSender;
};