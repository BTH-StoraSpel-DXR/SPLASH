#pragma once
#include <memory>
#include <bitset>

#define MAX_NUM_COMPONENTS_TYPES 128


/*
	Any component of type T created should inherit from Component<T>.
	Their IDs will automatically be assigned, and can be accessed via T::ID.
	They must have a default constructor, but can have default arguments or other constructors as well.
	No logic SHOULD be within each component, only raw data.

	Example: See MovementComponent.h
*/



typedef int ComponentTypeID;
typedef std::bitset<MAX_NUM_COMPONENTS_TYPES> ComponentTypeBitID;

/*
	Counter for assigning IDs to component types at compile time
	Defined in Component.cpp
*/
extern ComponentTypeID global_componentID;

/*
	Base component class
	Created components should NOT inherit from this
	This should be hidden in the future
*/
class BaseComponent {
public:
	typedef std::unique_ptr<BaseComponent> Ptr;

	virtual ~BaseComponent() {}
	
	/*
		Called by Component<T> below to assign IDs to the different types
		Should not be called anywhere else
	*/
	static ComponentTypeID createID() {
		return global_componentID++;
	}

	/*
		Retrieves the number of component types
	*/
	static int nrOfComponentTypes() {
		return global_componentID;
	}

	unsigned int entityID;

protected:
	BaseComponent() {}
};


/*
	Component class
	Created components should inherit from this and pass their own type as template
	Their type ID will be assigned automatically
*/
template<typename ComponentType>
class Component : public BaseComponent {
public:
	virtual ~Component() {}

	static const ComponentTypeID ID;
	static const ComponentTypeBitID& getBID() {
		static ComponentTypeBitID BID = static_cast<ComponentTypeBitID>(1ULL << ComponentType::ID);
		return BID;
	}
protected:
	Component() {}
};

/*
	Defines the constant static ID of each component type at compile time
*/
template<typename ComponentType>
const ComponentTypeID Component<ComponentType>::ID = BaseComponent::createID();