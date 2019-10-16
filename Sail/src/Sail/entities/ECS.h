#pragma once
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include "Entity.h"
#include "systems/BaseComponentSystem.h"
#include "Sail/entities/EntityFactory.hpp"

class EntityAdderSystem;
class EntityRemovalSystem;

/*
	See BaseComponentSystem.h on how to create systems
	See Component.h on how to create components

	Simple usage instruction:
		Create systems and entities via ECS
		Manage them outside ECS, i.e.:
			Add and remove components from entities
			Update systems in any logical order
		
		NOTE: If the developer does NOT want an entity to be automatically added to a system even when it has the valid components,
			set entity.tryToAddToSystems = false.
		NOTE: Entities can be added to a system manually, if specific situations demands it.
			Call ecs->addEntityToSystems() to add it to every system it fits within.
			Call system->addEntity() to add it to a specific system.
		NOTE: A system needs to exist before an entity can be added to it. It will NOT check each entity if created after them.

	Simple example:
		std::vector<Entity::SPtr> entities;
		ECS* ecs = ECS::Instance();
		PhysicSystem* ps = ecs->createSystem<PhysicSystem>();
		Entity::SPtr e = ecs->createEntity("FirstEntity");
		e->addComponent<TransformComponent>();
		e->addComponent<MovementComponent>(glm::vec3(1, 2, 3));
		entities.push_back(e);


	For any further questions, reach out to Samuel
*/

class ECS final {
public:

	/*
		Retrieves a pointer to an instance of this class
		Needs to be called to access ECS unless a direct pointer is stored
	*/
	static ECS* Instance() {
		static ECS instance;
		return &instance;
	}

	/*
		Creates and adds an entity
	*/
	Entity::SPtr createEntity(const std::string& name = "");

	/*
		Destroys all entities and their components
		stopAllSystems() should be called before this,
		to prevent an entity from being destroyed while still in use by a system
	*/
	void destroyAllEntities();

	/*
		Adds an already existing system of a chosen type
		Will not add if one of the same type already has been added or created
	*/
	template<typename T>
	void addSystem(T* system);

	/*
		Creates and adds a system of a chosen type
		Will not create if one of the same type already has been added or created
	*/
	template<typename T>
	T* createSystem();

	/*
		Retrieves a system of the chosen type if it has been created or added
		Otherwise a null pointer is returned
	*/
	template<typename T>
	T* getSystem();

	// Calls each system's 'stop()' function
	void stopAllSystems();

	// Clears the map of all unique_ptr systems
	void destroyAllSystems();

	/*
		Retrieves the entity adder system
		This is a special case system, every system should not have their own get-function
	*/
	EntityAdderSystem* getEntityAdderSystem();

	/*
		Retrieves the entity removal system
		This is a special case system, every system should not have their own get-function
	*/
	EntityRemovalSystem* getEntityRemovalSystem();

	/*
		Returns the number of unique component types
	*/
	unsigned nrOfComponentTypes() const;

	/*
		Should NOT be called by the game developer every update
		This is meant to be called internally by EntityAdderSystem

	*/
	void addAllQueuedEntities();

private:
	typedef std::unordered_map<std::type_index, std::unique_ptr<BaseComponentSystem>> SystemMap;
	friend class EntityRemovalSystem;
	friend class Entity;

	ECS();
	~ECS();

	// Internal adding of entities to systems, called by Entity
	void addEntityToSystems(Entity* entity);
	
	// Internal removal of entities to systems, called by Entity
	void removeEntityFromSystems(Entity* entity);

	// Internal queued destruction of entities, called by Entity
	void queueDestructionOfEntity(Entity* entity);

	// Internal destruction of entities, called by EntityRemovalSystem
	void destroyEntity(const Entity::SPtr entityToRemove);
	void destroyEntity(int ecsIndex);

private:
	std::vector<Entity::SPtr> m_entities;
	SystemMap m_systems;

	// This system is a special case, entities should never be automatically added to this when adding a component
	EntityAdderSystem* m_entityAdderSystem;
	// This system is a special case, entities should never be automatically added to this when adding a component
	EntityRemovalSystem* m_entityRemovalSystem;
};

template<typename T>
inline void ECS::addSystem(T* system) {
	SystemMap::iterator it = m_systems.find(typeid(T));
	if (it == m_systems.end()) {
		m_systems[typeid(T)] = std::unique_ptr<T>(system);
	}
}

template<typename T>
inline T* ECS::createSystem() {
	SystemMap::iterator it = m_systems.find(typeid(T));
	if (it == m_systems.end()) {
		m_systems[typeid(T)] = std::make_unique<T>();
	}
	return static_cast<T*>(m_systems.at(typeid(T)).get());
}

template<typename T>
inline T* ECS::getSystem() {
	SystemMap::iterator it = m_systems.find(typeid(T));
	if (it == m_systems.end()) {
		return nullptr;
	}
	return static_cast<T*>(it->second.get());
}