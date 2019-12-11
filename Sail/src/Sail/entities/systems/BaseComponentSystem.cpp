#include "pch.h"
#include "BaseComponentSystem.h"
#include "..//Entity.h"

bool BaseComponentSystem::addEntity(Entity* entity) {
	
	// Check if the entity is in the system

	int id = entity->getID();
	if (entities_set.count(id)) {
		return false;
	}

	// Check if the entity is about to be in the system.
	if (entitiesQueuedToAdd_set.count(id)) {
		return false;
	}

	// Queue the adding of the entity 
	entitiesQueuedToAdd.push_back(entity);
	entitiesQueuedToAdd_set.insert(id);

	return true;
}

// Please, never use me
bool BaseComponentSystem::instantAddEntity(Entity* entity) {
	// Check if the entity is in the system
	int id = entity->getID();
	if (entities_set.count(id)) {
		return false;
	}

	// Check if the entity is about to be in the system
	if (entitiesQueuedToAdd_set.count(id)) {
		return false;
	}

	entities.push_back(entity);
	entities_set.insert(entity->getID());

	return true;
}

void BaseComponentSystem::removeEntity(Entity* entity) {
	entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
	entitiesQueuedToAdd.erase(std::remove(entitiesQueuedToAdd.begin(), entitiesQueuedToAdd.end(), entity), entitiesQueuedToAdd.end());

	entitiesQueuedToAdd_set.erase(entity->getID());
	entities_set.erase(entity->getID());
}

const std::bitset<MAX_NUM_COMPONENTS_TYPES>& BaseComponentSystem::getRequiredComponentTypes() const {
	return requiredComponentTypes;
}

const std::bitset<MAX_NUM_COMPONENTS_TYPES>& BaseComponentSystem::getReadBitMask() const {
	return readBits;
}

const std::bitset<MAX_NUM_COMPONENTS_TYPES>& BaseComponentSystem::getWriteBitMask() const {
	return writeBits;
}

void BaseComponentSystem::clearEntities() {
	entities.clear();
	entities_set.clear();

	entitiesQueuedToAdd.clear();
	entitiesQueuedToAdd_set.clear();
}

size_t BaseComponentSystem::getNumEntities() {
	return entities.size();
}
#ifdef DEVELOPMENT
const std::vector<Entity*>& BaseComponentSystem::getEntities() const{
	return entities;
}
void BaseComponentSystem::imguiPrint(Entity** selectedEntity) {
}
unsigned int BaseComponentSystem::getByteSize() const {
	unsigned int size = entities.size() * sizeof(Entity*);
	size += entitiesQueuedToAdd.size() * sizeof(Entity*);
	size += entities_set.size() * sizeof(int);
	size += entitiesQueuedToAdd_set.size() * sizeof(int);
	return size;
}
#endif
void BaseComponentSystem::addQueuedEntities() {
	for (Entity* e : entitiesQueuedToAdd) {
		entities_set.insert(e->getID());
	}

	entities.insert(entities.end(), entitiesQueuedToAdd.begin(), entitiesQueuedToAdd.end());
	entitiesQueuedToAdd.clear();
	entitiesQueuedToAdd_set.clear();
}
