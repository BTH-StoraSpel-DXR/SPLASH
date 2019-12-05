#pragma once
#include "Sail/events/Event.h"
#include "Sail/netcode/NetcodeTypes.h"

class Entity;

struct StartKillCamEvent : public Event {

	StartKillCamEvent(const Netcode::ComponentID projectile, const Netcode::PlayerID killed, const bool finalKill = false)
		: Event(Event::Type::START_KILLCAM), killingProjectile(projectile), deadPlayer(killed), finalKillCam(finalKill)
	{}

	Netcode::ComponentID killingProjectile;
	Netcode::PlayerID deadPlayer;

	bool finalKillCam;

};