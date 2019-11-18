#pragma once
#include "Event.h"

struct UpdateSanityEvent : public Event {
	UpdateSanityEvent(const int _id, const float _sanity)
		: Event(Event::Type::SANITY_SYSTEM_UPDATE_SANITY)
		, id(_id) 
		, sanityVal(_sanity)
		{}
	~UpdateSanityEvent() = default;

	const int id;
	const float sanityVal;
};