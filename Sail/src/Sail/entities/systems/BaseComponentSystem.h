#pragma once

#include "../components/Component.h"

#include <bitset>

class Entity;

/*
	Created systems must inherit from this class.
	In the sub-system's default constructor (or if it has an initialize function),
	add each component type required for that system to m_requiredComponentTypes.
	Any entity in m_entities WILL have all of the required components, so no checks are needed.
	However, there can still be optional components if the system wants it, which should be checked before used.

	Example: See PhysicSystem.h and PhysicSystem.cpp
*/

class BaseComponentSystem {
public:
	BaseComponentSystem()
		: requiredComponentTypes(0x0)
		, readBits(0x0)
		, writeBits(0x0) {}
	virtual ~BaseComponentSystem() {}

	virtual void update(float dt) = 0;

	/*
		Adds an entity to the system
	*/
	virtual bool addEntity(Entity* entity);

	/*
		Removes an entity from the system
	*/
	virtual void removeEntity(Entity* entity);

	/*
		Returns the indices of all the component types required to be within this system
	*/
	const std::bitset<MAX_NUM_COMPONENTS_TYPES>& getRequiredComponentTypes() const;

	/* Returns the bit mask for all components that are being read from */
	const std::bitset<MAX_NUM_COMPONENTS_TYPES>& getReadBitMask() const;

	/* Returns the bit mask for all components that are being written to */
	const std::bitset<MAX_NUM_COMPONENTS_TYPES>& getWriteBitMask() const;

	virtual void stop() {}
	virtual void clearEntities();
	virtual size_t getNumEntities();

	// Do not call this, it is called internally by EntityAdderSystem
	void addQueuedEntities();

protected:
	/**
	 * Registers the component to the system and defines how the system uses the component
	 *
	 * @param required Should be set to true if the component is required by the system
	 * @param read Should be set to true if the component is read by the system
	 * @param write Should be set to true if the component is written to by the system
	 */
	template<typename ComponentType>
	void registerComponent(bool required, bool read, bool write);

protected:
	std::vector<Entity*> entities;
	std::vector<Entity*> entitiesQueuedToAdd;

	std::bitset<MAX_NUM_COMPONENTS_TYPES> requiredComponentTypes;
	std::bitset<MAX_NUM_COMPONENTS_TYPES> readBits;
	std::bitset<MAX_NUM_COMPONENTS_TYPES> writeBits;
};

template<typename ComponentType>
inline void BaseComponentSystem::registerComponent(bool required, bool read, bool write) {
	if ( required ) { 
		requiredComponentTypes |= ComponentType::getBID(); 
	}
	if ( read ) { 
		readBits |= ComponentType::getBID(); 
	}
	if ( write ) { 
		writeBits |= ComponentType::getBID(); 
	}
}
