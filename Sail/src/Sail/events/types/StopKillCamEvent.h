#pragma once
#include "Sail/events/Event.h"

struct StopKillCamEvent : public Event {
	StopKillCamEvent(bool finalKillCam) : Event(Event::Type::STOP_KILLCAM), isFinalKill(finalKillCam) {}

	bool isFinalKill;
};