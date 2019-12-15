#include "pch.h"
#include "OctreeAddRemoverSystem.h"

#include "../Physics/Physics.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/components/RenderInActiveGameComponent.h"
#include "Sail/entities/components/RenderInReplayComponent.h"
#include "Sail/graphics/geometry/Model.h"
#include "Sail/graphics/camera/Camera.h"

template <typename T>
OctreeAddRemoverSystem<T>::OctreeAddRemoverSystem() 
	: m_doCulling(false)
	, m_cullCamera(nullptr)
{
	// TODO: System owner should check if this is correct
	registerComponent<BoundingBoxComponent>(true, true, true);
	registerComponent<CollidableComponent>(true, true, true);
	registerComponent<T>(true, false, false);
}

template <typename T>
OctreeAddRemoverSystem<T>::~OctreeAddRemoverSystem() {

}

template <typename T>
void OctreeAddRemoverSystem<T>::provideOctree(Octree* octree) {
	m_octree = octree;
	m_octree->addEntities(&entities);
}

template <typename T>
bool OctreeAddRemoverSystem<T>::addEntity(Entity* entity) {
	if (BaseComponentSystem::addEntity(entity) && m_octree) {
			m_octree->addEntity(entity);
			return true;
	}
	return false;
}

template <typename T>
void OctreeAddRemoverSystem<T>::removeEntity(Entity* entity) {
	BaseComponentSystem::removeEntity(entity);
	//entity->queueDestruction();

	if (m_octree) {
		m_octree->removeEntity(entity);
	}
}


template <typename T>
void OctreeAddRemoverSystem<T>::update(float dt) {
	m_octree->update();
}

template <typename T>
void OctreeAddRemoverSystem<T>::updatePerFrame(float dt) {
	if (m_doCulling) {
		// Let the renderer know that all entities should not be rendered - will be set to true in cull method call if they are visible
		for (auto& entity : entities) {
			auto* cullComponent = entity->getComponent<CullingComponent>();
			if (cullComponent) {
				cullComponent->isVisible = false;
			}
		}
		m_octree->frustumCulledDraw(*m_cullCamera);
	}
}

template <typename T>
void OctreeAddRemoverSystem<T>::setCulling(bool activated, Camera* camera) {
	m_doCulling = activated;
	m_cullCamera = camera;
}

#ifdef DEVELOPMENT
template <typename T>
unsigned int OctreeAddRemoverSystem<T>::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif

template class OctreeAddRemoverSystem<RenderInActiveGameComponent>;
template class OctreeAddRemoverSystem<RenderInReplayComponent>;