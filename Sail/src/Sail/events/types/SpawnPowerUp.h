#pragma once
#include "Sail/events/Event.h"
#include "Sail/netcode/NetcodeTypes.h"
#include "Sail/utils/Utils.h"

struct SpawnPowerUp : public Event {
	SpawnPowerUp(int powerUpType, glm::vec3 pos, Netcode::ComponentID _netCompID, Entity* parentEntity)
		: Event(Event::Type::SPAWN_POWERUP)
		, powerUpType(powerUpType)
		, pos(pos)
		, _netCompID(_netCompID)
		, parentEntity(parentEntity)
	{
	}

	int powerUpType;
	glm::vec3 pos;
	Netcode::ComponentID _netCompID;
	Entity* parentEntity;
};

struct DestroyPowerUp : public Event {
	DestroyPowerUp(Netcode::ComponentID _netCompID, Netcode::ComponentID pickedByPlayer)
		: Event(Event::Type::DESTROY_POWERUP)
		, _netCompID(_netCompID)
		, pickedByPlayer(pickedByPlayer)
	{
	}

	Netcode::ComponentID _netCompID;
	Netcode::ComponentID pickedByPlayer;
};