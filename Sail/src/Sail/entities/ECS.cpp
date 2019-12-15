#include "pch.h"
#include "ECS.h"
#include "systems/entityManagement/EntityAdderSystem.h"
#include "systems/entityManagement/EntityRemovalSystem.h"
#include "systems/Audio/AudioSystem.h"
#include "Sail.h"

void ECS::addAllQueuedEntities() {
	for (auto& s : m_systems) {
		if (s.second) {
			s.second->addQueuedEntities();
		}
	}
}

ECS::ECS() {
	// Add the special case systems
	m_entityAdderSystem = SAIL_NEW EntityAdderSystem();
	m_entityRemovalSystem = SAIL_NEW EntityRemovalSystem();
}

ECS::~ECS() {
	delete m_entityRemovalSystem;
	delete m_entityAdderSystem;
}

void ECS::stopAllSystems() {
	for (auto& sys : m_systems) {
		if (sys.second) {
			sys.second->stop();
			sys.second->clearEntities();
		}
	}
}

void ECS::destroyAllSystems() {
	m_systems.clear();
}

EntityAdderSystem* ECS::getEntityAdderSystem() {
	return m_entityAdderSystem;
}

EntityRemovalSystem* ECS::getEntityRemovalSystem() {
	return m_entityRemovalSystem;
}

unsigned ECS::nrOfComponentTypes() const {
	return BaseComponent::nrOfComponentTypes();
}

Entity::SPtr ECS::createEntity(const std::string& name) {
	m_entities.push_back(Entity::Create(this, name));
	m_entities.back()->setECSIndex((int)m_entities.size() - 1);
	return m_entities.back();
}

void ECS::queueDestructionOfEntity(Entity* entity) {
	// Add entity to removal system
	m_entityRemovalSystem->addEntity(entity);
	
	// Might be a poor solution
	m_entityRemovalSystem->addQueuedEntities();
}

void ECS::destroyEntity(const Entity::SPtr entityToRemove) {
	// Find the index of the entity in the vector
	destroyEntity(entityToRemove->getECSIndex());
}

void ECS::destroyEntity(int ecsIndex) {
	// Remove all of the entity's components
	// Also removes it from the systems
	m_entities[ecsIndex]->removeAllComponents();

	// Remove from parents and children
	m_entities[ecsIndex]->removeAllChildren();
	if (m_entities[ecsIndex]->getParent()) {
		m_entities[ecsIndex]->getParent()->removeChildEntity(m_entities[ecsIndex].get());
	}

	// Move the last entity in the vector
	m_entities[ecsIndex] = m_entities.back();

	// Set the index of the moved entity
	m_entities[ecsIndex]->setECSIndex(ecsIndex);

	// Remove the redundant copy of the moved entity
	m_entities.pop_back();
}

void ECS::destroyAllEntities() {
	// TODO: Probably also needs to be updated to use destroyEntity instead
	for (auto& e : m_entities) {
		//e->removeAllComponents();	// Also removes them from systems
		destroyEntity(e);
	}

	m_entities.clear();
}

void ECS::addEntityToSystems(Entity* entity) {
	SystemMap::iterator it = m_systems.begin();

	// Check which systems this entity can be placed in
	for ( ; it != m_systems.end(); ++it ) {
		if (it->second) {
			auto componentTypes = it->second->getRequiredComponentTypes();

			// Add this entity to the system
			if (entity->hasComponents(componentTypes)) {
				it->second->addEntity(entity);
			}
		}
	}
}

void ECS::removeEntityFromSystems(Entity* entity) {
	SystemMap::iterator it = m_systems.begin();

	for ( ; it != m_systems.end(); ++it ) {
		if (it->second) {
			auto componentTypes = it->second->getRequiredComponentTypes();

			if (!entity->hasComponents(componentTypes)) {
				it->second->removeEntity(entity);
			}
		}
	}
}

size_t ECS::getNumEntities() {
	return m_entities.size();
}
#ifdef DEVELOPMENT
const ECS::SystemMap& ECS::getSystems() const {
	return m_systems;
}

const unsigned int ECS::getByteSize() const {
	unsigned int size = sizeof(*this);
	size += m_entities.size() * sizeof(Entity::SPtr);
	size += m_systems.size() * sizeof(std::pair< std::type_index, std::unique_ptr<BaseComponentSystem>>);
	
	for (auto& sys : m_systems) {
		if (sys.second) {
			size += sys.second->getByteSize();
		}
	}
	size += m_entityRemovalSystem->getByteSize();
	size += m_entityAdderSystem->getByteSize();

	return size;
}

const unsigned int ECS::getByteSizeComponents() const {
	unsigned int size = 0;
	for (auto e : m_entities) {
		const BaseComponent::Ptr* components = e->getComponents();
		for (unsigned int index = 0; index < BaseComponent::nrOfComponentTypes(); index++) {
			if (BaseComponent* ptr = components[index].get()) {
				size += ptr->getByteSize();
			}
		}
	}

	return size;
}
#endif