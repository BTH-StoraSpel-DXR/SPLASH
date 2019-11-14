#include "pch.h"
#include "Entity.h"
#include "ECS.h"

#include "components/TransformComponent.h"

static int s_id = 0;

Entity::SPtr Entity::Create(ECS* ecs, const std::string& name) {
	Entity::SPtr temp = std::make_shared<Entity>(name);
	temp->m_ecs = ecs;
	return temp;
}

void Entity::addToSystems() {
	m_ecs->addEntityToSystems(this);
}

void Entity::removeFromSystems() {
	m_ecs->removeEntityFromSystems(this);
}

void Entity::setECSIndex(int index) {
	m_ECSIndex = index;
}

void Entity::setParent(Entity* entity) {
	m_parent = entity;
}

int Entity::getECSIndex() const {
	return m_ECSIndex;
}

Entity::Entity(const std::string& name) 
	: m_componentTypes(0x0),
	m_name(name),
	m_parent(nullptr)
{
	m_id = s_id++;
	m_ECSIndex = -1;

	m_components = SAIL_NEW BaseComponent::Ptr[BaseComponent::nrOfComponentTypes()];
}

Entity::~Entity() {
	delete[] m_components;
}

bool Entity::hasComponents(std::bitset<MAX_NUM_COMPONENTS_TYPES> componentTypes) const {
	return (m_componentTypes & componentTypes) == componentTypes;
}
#ifdef DEVELOPMENT
const BaseComponent::Ptr* Entity::getComponents() const {
	return m_components;
}
#endif
Entity* Entity::getParent() {
	return m_parent;
}

bool Entity::isAboutToBeDestroyed() const {
	return m_destructionQueued;
}

void Entity::queueDestruction() {
	m_destructionQueued = true;
	m_ecs->queueDestructionOfEntity(this);
}

void Entity::removeComponent(ComponentTypeID id) {
	const ComponentTypeBitID bid = GetBIDofID(id);

	if ((m_componentTypes & bid).any()) {
		m_components[id].reset();

		// Set the component type bit to 0 if it was 1
		m_componentTypes ^= bid;

		// Remove this entity from systems which required the removed component
		removeFromSystems();
	}
}

// TODO: should only be able to be called on entities with m_destructionQueued == true
void Entity::removeAllComponents() {
	for (int i = 0; i < BaseComponent::nrOfComponentTypes(); i++) {
		m_components[i].reset(nullptr);
	}
	//m_components.clear();
	m_componentTypes = std::bitset<MAX_NUM_COMPONENTS_TYPES>(0);
	removeFromSystems();
}

void Entity::addChildEntity(Entity* child) {
	m_children.push_back(child);
	child->setParent(this);

	auto transComp = getComponent<TransformComponent>();
	if ( transComp ) {
		auto childTransComp = child->getComponent<TransformComponent>();
		if ( childTransComp ) {
			childTransComp->setParent(transComp);
		}
	}
}

void Entity::removeChildEntity(Entity* toRemove) {
	auto child = std::find(m_children.begin(), m_children.end(), toRemove);
	( *child )->setParent(nullptr);
	if ( ( *child )->hasComponent<TransformComponent>() ) {
		auto childTransComp = ( *child )->getComponent<TransformComponent>();
		childTransComp->removeParent();
	}	
	m_children.erase(child);
}

void Entity::removeAllChildren() {

	for ( auto child : m_children ) {
		if ( child->hasComponent<TransformComponent>() ) {
			auto childTransComp = child->getComponent<TransformComponent>();
			childTransComp->removeParent();
		}
	}
	m_children.clear();
}

void Entity::removeDeleteAllChildren() {
	for (auto child : m_children) {

		child->queueDestruction();
		if (child->hasComponent<TransformComponent>()) {

			auto childTransComp = child->getComponent<TransformComponent>();
			childTransComp->removeParent();
		}
	}
	m_children.clear();
}

std::vector<Entity*>& Entity::getChildEntities() {
	return m_children;
}

void Entity::setName(const std::string& name) {
	m_name = name;
}

const std::string& Entity::getName() const {
	return m_name;
}

int Entity::getID() const {
	return m_id;
}
