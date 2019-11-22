#include "pch.h"
#include "AiSystem.h"
#include "../../../components/AiComponent.h"
#include "../../../components/TransformComponent.h"
#include "../../../components/FSMComponent.h"
#include "../../../components/MovementComponent.h"
#include "../../../components/SpeedLimitComponent.h"
#include "Sail/ai/pathfinding/NodeSystem.h"

#include "../../../ECS.h"
#include "../../../components/BoundingBoxComponent.h"
#include "../../physics/UpdateBoundingBoxSystem.h"
#include "../../physics/OctreeAddRemoverSystem.h"
#include "Sail/utils/Utils.h"
#include "../../Physics/Octree.h"
#include "Sail/Application.h"
#include "../../Physics/Intersection.h"
#include "../../Physics/Physics.h"

AiSystem::AiSystem() {
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<SpeedLimitComponent>(true, true, true);
	registerComponent<AiComponent>(true, true, true);
	registerComponent<FSMComponent>(true, true, true);

	m_nodeSystem = std::make_unique<NodeSystem>();
	m_timeBetweenPathUpdate = 0.5f;
}

AiSystem::~AiSystem() {}

#ifdef _DEBUG_NODESYSTEM
void AiSystem::initNodeSystem(Model* bbModel, Octree* octree, Shader* shader) {
	m_nodeSystem->setDebugModelAndScene(shader);
	initNodeSystem(bbModel, octree);
}
#endif

void AiSystem::initNodeSystem(Model* bbModel, Octree* octree) {

	std::vector<NodeSystem::Node> nodes;
	std::vector<std::vector<unsigned int>> connections;

	m_octree = octree;

	std::vector<unsigned int> conns;
	int x_max = 5*7;
	int z_max = 5*7;
	int x_cur = 0;
	int z_cur = 0;
	int size = x_max * z_max;

	int padding = 2;
	float offsetX = -5.f;
	float offsetZ = -5.f;
	float offsetY = 1.f;
	bool* walkable = SAIL_NEW bool[size];

	auto e = ECS::Instance()->createEntity("DeleteMeFirstFrameDummy");
	e->addComponent<BoundingBoxComponent>(bbModel)->getBoundingBox()->setHalfSize(glm::vec3(0.7f, .9f, 0.7f));


	/*Nodesystem*/
	ECS::Instance()->getSystem<UpdateBoundingBoxSystem>()->update(0.f);
	ECS::Instance()->getSystem<OctreeAddRemoverSystem>()->update(0.f);
	for ( size_t i = 0; i < size; i++ ) {
		conns.clear();
		x_cur = i % x_max;
		z_cur = static_cast<int>(floor(i / x_max));
		glm::vec3 pos(x_cur * padding + offsetX, offsetY, z_cur * padding + offsetZ);

		bool blocked = false;
		BoundingBox* bb= e->getComponent<BoundingBoxComponent>()->getBoundingBox();
		bb->setPosition(pos);
		std::vector<Octree::CollisionInfo> vec;
		m_octree->getCollisions(e.get(), bb, &vec);

		for (Octree::CollisionInfo& info : vec ) {
			int j = ( info.entity->getName().compare("Map_") );
			if ( j >= 0 ) {
				//Not walkable

				blocked = true;
				break;
			}
		}

		nodes.emplace_back(pos, blocked, i);

		for ( int dx = -1; dx <= 1; dx++ ) {
			for ( int dz = -1; dz <= 1; dz++ ) {
				if ( dx == 0 && dz == 0 )
					continue;

				int nx = x_cur + dx;
				int nz = z_cur + dz;
				if ( nx >= 0 && nx < x_max && nz >= 0 && nz < z_max ) {
					int ni = nx + nz * x_max;
					conns.push_back(ni);
				}
			}
		}

		connections.push_back(conns);
	}
	//Delete "DeleteMeFirstFrameDummy"
	e->queueDestruction();

	m_nodeSystem->setNodes(nodes, connections);
	Memory::SafeDeleteArr(walkable);
}

std::vector<Entity*>& AiSystem::getEntities() {
	return entities;
}

void AiSystem::update(float dt) {
	std::vector<std::future<void>> futures;
	for ( auto& entity : entities ) {
		// Might be dangerous for threads
		futures.push_back(Application::getInstance()->pushJobToThreadPool([this, entity, dt] (int id) { this->aiUpdateFunc(entity, dt); }));
	}
	for ( auto& a : futures ) {
		a.get();
	}
}

NodeSystem* AiSystem::getNodeSystem() {
	return m_nodeSystem.get();
}

#ifdef DEVELOPMENT
unsigned int AiSystem::getByteSize() const {
	unsigned int size = sizeof(*this);
	size += m_nodeSystem->getByteSize();
	return size;
}
#endif

void AiSystem::aiUpdateFunc(Entity* e, const float dt) {
	e->getComponent<FSMComponent>()->update(dt, e);
	
	AiComponent* ai = e->getComponent<AiComponent>();

	if ( ai->timeTakenOnPath > m_timeBetweenPathUpdate && ai->doWalk ) {
		ai->updatePath = true;
	}

	updatePath(e);
	updatePhysics(e, dt);
}

glm::vec3 AiSystem::getDesiredDir(AiComponent* aiComp, TransformComponent* transComp) {
	glm::vec3 desiredDir = aiComp->currPath[aiComp->currNodeIndex].position - transComp->getTranslation();
	if ( desiredDir == glm::vec3(0.f) ) {
		desiredDir = glm::vec3(1.0f, 0.f, 0.f);
	}
	desiredDir = glm::normalize(desiredDir);
	return desiredDir; // TODO: Check this - should probably not return a reference??
}


void AiSystem::updatePath(Entity* e) {
	AiComponent* ai = e->getComponent<AiComponent>();
	TransformComponent* transform = e->getComponent<TransformComponent>();
	if (ai->updatePath ) {

		ai->timeTakenOnPath = 0.f;
		ai->reachedPathingTarget = false;

		// ai->posTarget is updated in each FSM state
		ai->lastTargetPos = ai->posTarget;

		auto tempPath = m_nodeSystem->getPath(transform->getTranslation(), ai->posTarget);

		ai->currNodeIndex = 0;

		// Fix problem of always going toward closest node
		if ( tempPath.size() > 1 && glm::distance(tempPath[1].position, transform->getTranslation()) < glm::distance(tempPath[1].position, tempPath[0].position) ) {
			ai->currNodeIndex += 1;
		}

		ai->currPath = tempPath;

		ai->updatePath = false;
	}
}

void AiSystem::updatePhysics(Entity* e, float dt) {
	AiComponent* ai = e->getComponent<AiComponent>();
	MovementComponent* movement = e->getComponent<MovementComponent>();
	TransformComponent* transform = e->getComponent<TransformComponent>();
	SpeedLimitComponent* speedLimit = e->getComponent<SpeedLimitComponent>();

	// Check if there is a path currently active and if the ai should be walking
	if ( ai->currPath.size() > 0 && ai->doWalk ) {
		// Check if the ai hasn't reached the current pathing target yet
		if ( !ai->reachedPathingTarget ) {
			ai->timeTakenOnPath += dt;

			// Check if the distance between current node target and ai is low enough to begin targeting next node
			if ( glm::distance(transform->getTranslation(), ai->currPath[ai->currNodeIndex].position) < ai->targetReachedThreshold ) {
				ai->lastVisitedNode = ai->currPath[ai->currNodeIndex];
				ai->reachedPathingTarget = true;
			// Else continue walking
			} else {
				float acceleration = 70.0f - ( glm::length(movement->velocity) / speedLimit->maxSpeed ) * 20.0f;
				movement->accelerationToAdd = getDesiredDir(ai, transform) * acceleration;
			}
		// Else increment the current node path
		} else if (ai->currPath.size() > 0 ) {
			// Update next node target
			if (ai->currNodeIndex < ai->currPath.size() - 1 ) {
				ai->currNodeIndex++;
			}
			ai->reachedPathingTarget = false;
		}

		float newYaw = getAiYaw(movement, transform->getRotations().y, dt);
		transform->setRotations(0.f, newYaw, 0.f);

	// If the ai shouldn't walk, just stop walking
	} else {
		// Set velocity to 0
		movement->velocity = glm::vec3(0.f, movement->velocity.y, 0.f);
		ai->timeTakenOnPath = 3.f;
	}
}

float AiSystem::getAiYaw(MovementComponent* moveComp, float currYaw, float dt) {
	float newYaw = currYaw;
	if ( glm::length2(moveComp->velocity) > 0.f ) {
		float desiredYaw = 0.f;
		float turnRate = glm::two_pi<float>() / 2.f; // 2 pi
		auto normalizedVel = glm::normalize(moveComp->velocity);
		float moveCompX = normalizedVel.x;
		float moveCompZ = normalizedVel.z;
		moveCompZ = moveCompZ != 0 ? moveCompZ : 0.1f;
		if (moveComp->velocity.z < 0.f ) {
			desiredYaw = glm::atan(moveCompX / moveCompZ) + 1.5707f;
		} else {
			desiredYaw = glm::atan(moveCompX / moveCompZ) - 1.5707f;
		}
		desiredYaw = Utils::wrapValue(desiredYaw, 0.f, glm::two_pi<float>());
		float diff = desiredYaw - currYaw;

		if ( std::abs(diff) > glm::pi<float>() ) {
			diff = currYaw - desiredYaw;
		}

		float toTurn = 0.f;
		if ( diff > 0 ) {
			toTurn = turnRate * dt;
		} else if ( diff < 0 ) {
			toTurn = -turnRate * dt;
		}

		if ( std::abs(diff) > std::abs(toTurn) ) {
			newYaw = Utils::wrapValue(currYaw + toTurn, 0.f, glm::two_pi<float>());
		} else {
			newYaw = Utils::wrapValue(currYaw + diff, 0.f, glm::two_pi<float>());
		}

	}
	return newYaw;
}
