#pragma once
#include "Sail/events/Event.h"

class Entity;

enum class SlowMotionSetting : int {
	ENABLE,
	DISABLE
};

struct ToggleSlowMotionReplayEvent : public Event {
	ToggleSlowMotionReplayEvent(const SlowMotionSetting _setting) : Event(Event::Type::TOGGLE_SLOW_MOTION), setting(_setting) {}

	SlowMotionSetting setting;
};