#include "pch.h"
#include "CollisionSystem.h"
#include "..//..//components/TransformComponent.h"
#include "..//..//components/MovementComponent.h"
#include "..//..//components/CollisionComponent.h"
#include "..//..//components/BoundingBoxComponent.h"
#include "..//..//components/CollisionSpheresComponent.h"
#include "..//..//Physics/Intersection.h"

#include "Sail/Application.h"

CollisionSystem::CollisionSystem() {
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<CollisionComponent>(true, true, true);
	registerComponent<BoundingBoxComponent>(true, true, true);
	registerComponent<CollisionSpheresComponent>(false, true, false);

	m_octree = nullptr;
}

CollisionSystem::~CollisionSystem() {
}

void CollisionSystem::provideOctree(Octree* octree) {
	m_octree = octree;
}


void CollisionSystem::update(float dt) {
	constexpr size_t NR_OF_JOBS = 16;
	constexpr size_t LAST_JOB = NR_OF_JOBS - 1;
	const size_t entitiesPerJob = entities.size() / NR_OF_JOBS;
	std::future<bool> jobs[NR_OF_JOBS];


	// prepare matrixes and bounding boxes
	for (auto e : entities) {
		e->getComponent<TransformComponent>()->prepareMatrix();
		e->getComponent<BoundingBoxComponent>()->getBoundingBox()->prepareCorners();
	}

	// ======================== Collision Update ======================================
	
	// Start executing jobs
	for (size_t i = 0; i < NR_OF_JOBS - 1; ++i) {
		jobs[i] = Application::getInstance()->pushJobToThreadPool([=](int id) {
			return collisionUpdatePart(dt, i * entitiesPerJob, i * entitiesPerJob + entitiesPerJob);
		});
	}
	jobs[LAST_JOB] = Application::getInstance()->pushJobToThreadPool([=](int id) {
		return collisionUpdatePart(dt, LAST_JOB * entitiesPerJob, entities.size());
	});

	// Wait for jobs to finish executing
	for (size_t i = 0; i < NR_OF_JOBS; ++i) { jobs[i].get(); }


	// ======================== Surface from collisions ======================================

	// Start executing jobs
	for (size_t i = 0; i < NR_OF_JOBS - 1; ++i) {
		jobs[i] = Application::getInstance()->pushJobToThreadPool([=](int id) {
			return surfaceFromCollisionPart(dt, i * entitiesPerJob, i * entitiesPerJob + entitiesPerJob);
		});
	}
	jobs[LAST_JOB] = Application::getInstance()->pushJobToThreadPool([=](int id) {
		return surfaceFromCollisionPart(dt, LAST_JOB * entitiesPerJob, entities.size());
	});

	// Wait for jobs to finish executing
	for (size_t i = 0; i < NR_OF_JOBS; ++i) { jobs[i].get(); }


	// ======================== Ray cast collisions ======================================
	// Technically not thread safe but we presume that fast travelling objects (basically water) 
	// will not collide with other fast travelling objects

	// Start executing jobs
	for (size_t i = 0; i < NR_OF_JOBS - 1; ++i) {
		jobs[i] = Application::getInstance()->pushJobToThreadPool([=](int id) {
			return rayCastCollisionPart(dt, i * entitiesPerJob, i * entitiesPerJob + entitiesPerJob);
		});
	}
	jobs[LAST_JOB] = Application::getInstance()->pushJobToThreadPool([=](int id) {
		return rayCastCollisionPart(dt, LAST_JOB * entitiesPerJob, entities.size());
	});

	// Wait for jobs to finish executing
	for (size_t i = 0; i < NR_OF_JOBS; ++i) { jobs[i].get(); }
}

bool CollisionSystem::collisionUpdatePart(float dt, size_t start, size_t end) {
	for (size_t i = start; i < end; ++i) {
		Entity* e = entities[i];

		CollisionComponent*   collision   = e->getComponent<CollisionComponent>();
		BoundingBoxComponent* boundingBox = e->getComponent<BoundingBoxComponent>();
		const CollisionSpheresComponent* csc   = e->getComponent<CollisionSpheresComponent>();

		collision->collisions.clear();

		if (collision->padding < 0.0f) {
			glm::vec3 halfSize = boundingBox->getBoundingBox()->getHalfSize();
			collision->padding = glm::min(glm::min(halfSize.x, halfSize.y), halfSize.z);
		}

		if (m_octree && !csc) { //Not implemented for spheres yet
			collisionUpdate(e, dt);
		}
	}

	return true;
}

bool CollisionSystem::surfaceFromCollisionPart(float dt, size_t start, size_t end) {
	for (size_t i = start; i < end; ++i) {
		Entity* e = entities[i];

		CollisionComponent* collision = e->getComponent<CollisionComponent>();
		const CollisionSpheresComponent* csc = e->getComponent<CollisionSpheresComponent>();

		if (m_octree && !csc) { //Not implemented for spheres yet
			surfaceFromCollision(e, collision->collisions);
		}
	}
	return true;
}

bool CollisionSystem::rayCastCollisionPart(float dt, size_t start, size_t end) {
	for (size_t i = start; i < end; ++i) {
		Entity* e = entities[i];

		MovementComponent* movement = e->getComponent<MovementComponent>();
		BoundingBox* boundingBox = e->getComponent<BoundingBoxComponent>()->getBoundingBox();
		const CollisionSpheresComponent* csc = e->getComponent<CollisionSpheresComponent>();

		float updateableDt = dt;

		if (m_octree && !csc) {
			if (rayCastCheck(e, *boundingBox, updateableDt)) {
				//Object is moving fast, ray cast for collisions
				rayCastUpdate(e, *boundingBox, updateableDt);
				movement->oldVelocity = movement->velocity;
			}
		}
		movement->updateableDt = updateableDt;
	}
	return true;
}

const bool CollisionSystem::collisionUpdate(Entity* e, const float dt) {
	//Update collision data
	std::vector<Octree::CollisionInfo> collisions;
	CollisionComponent* collision = e->getComponent<CollisionComponent>();

	bool hasSpheres = e->hasComponent<CollisionSpheresComponent>();
	if (hasSpheres) {
		assert(false); //Not implemented
	} else {
		m_octree->getCollisions(e, &collisions, collision->doSimpleCollisions);
	}

	return handleCollisions(e, collisions, dt);
}


// Modifies Entity e's Movement- and CollisionComponent
// Neither of which is used in the rest of updatePart() so modifying them should be fine
const bool CollisionSystem::handleCollisions(Entity* e, std::vector<Octree::CollisionInfo>& collisions, const float dt) {
	bool collisionFound = false;

	MovementComponent* movement = e->getComponent<MovementComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();
	const BoundingBox* boundingBox = e->getComponent<BoundingBoxComponent>()->getBoundingBox();

	collision->onGround = false;

	const size_t collisionCount = collisions.size();

	if (collisionCount > 0) {
		std::vector<int> groundIndices;
		glm::vec3 sumVec(0.0f);
		std::vector<Octree::CollisionInfo> trueCollisions;

		//Get the actual intersection axises
		for (size_t i = 0; i < collisionCount; i++) {
			Octree::CollisionInfo& collisionInfo_i = collisions[i];

			glm::vec3 intersectionAxis;
			float intersectionDepth;

			if (collisionInfo_i.shape->getIntersectionDepthAndAxis(boundingBox, &intersectionAxis, &intersectionDepth)) {
				collisionInfo_i.intersectionAxis = intersectionAxis;

				sumVec += collisionInfo_i.intersectionAxis;

				collisionInfo_i.intersectionPosition = collisionInfo_i.shape->getIntersectionPosition(boundingBox);

				//Add collision to current collisions for collisionComponent
				collision->collisions.push_back(collisionInfo_i);

				//Add collision to true collisions
				trueCollisions.push_back(collisionInfo_i);

				collisionFound = true;
			}
		}

		//Loop through true collisions and handle them
		const size_t trueCollisionCount = trueCollisions.size();
		for (size_t i = 0; i < trueCollisionCount; i++) {
			const Octree::CollisionInfo& collisionInfo_i = trueCollisions[i];

			//Save ground collisions
			if (collisionInfo_i.intersectionAxis.y > 0.7f) {
				collision->onGround = true;
				bool newGround = true;
				for (size_t j = 0; j < groundIndices.size(); j++) {
					if (collisionInfo_i.intersectionAxis == trueCollisions[groundIndices[j]].intersectionAxis) {
						newGround = false;
					}
				}
				if (newGround) {
					//Save collision for friction calculation
					groundIndices.push_back((int)i);
				}
			}

			//----Velocity changes from collisions----

			//Stop movement towards triangle
			float projectionSize = glm::dot(movement->velocity, -collisionInfo_i.intersectionAxis);

			if (projectionSize > 0.0f) { //Is pushing against wall
				movement->velocity += collisionInfo_i.intersectionAxis * (projectionSize * (1.0f + collision->bounciness)); //Limit movement towards wall
			}


			//Tight angle corner special case
			const float dotProduct = glm::dot(collisionInfo_i.intersectionAxis, glm::normalize(sumVec));
			if (dotProduct < 0.7072f && dotProduct > 0.0f) { //Colliding in a tight angle corner
				glm::vec3 normalToNormal = sumVec - glm::dot(sumVec, collisionInfo_i.intersectionAxis) * collisionInfo_i.intersectionAxis;
				normalToNormal = glm::normalize(normalToNormal);

				//Stop movement towards corner
				projectionSize = glm::dot(movement->velocity, -normalToNormal);

				if (projectionSize > 0.0f) {
					movement->velocity += normalToNormal * projectionSize * (1.0f + collision->bounciness);
				}
			}
			//----------------------------------------
		}

		//----Drag----
		if (collision->onGround) { //Ground drag
			size_t nrOfGroundCollisions = groundIndices.size();
			for (size_t i = 0; i < nrOfGroundCollisions; i++) {
				const Octree::CollisionInfo& collisionInfo_ground_i = trueCollisions[groundIndices[i]];
				const glm::vec3 velAlongPlane = movement->velocity - collisionInfo_ground_i.intersectionAxis * glm::dot(collisionInfo_ground_i.intersectionAxis, movement->velocity);
				const float sizeOfVel = glm::length(velAlongPlane);
				if (sizeOfVel > 0.0f) {
					const float slowdown = glm::min((collision->drag / nrOfGroundCollisions) * dt, sizeOfVel);
					movement->velocity -= slowdown * glm::normalize(velAlongPlane);
				}
			}
		}
		//------------
	}

	return collisionFound;
}

const bool CollisionSystem::rayCastCheck(Entity* e, const BoundingBox& boundingBox, float& dt) const {
	const MovementComponent* movementComp = e->getComponent<MovementComponent>();

	if (glm::abs(movementComp->velocity.x * dt) > glm::abs(boundingBox.getHalfSize().x)
		|| glm::abs(movementComp->velocity.y * dt) > glm::abs(boundingBox.getHalfSize().y)
		|| glm::abs(movementComp->velocity.z * dt) > glm::abs(boundingBox.getHalfSize().z)) {
		//Object is moving at a speed that risks missing collisions
		return true;
	}
	return false;
}

void CollisionSystem::rayCastUpdate(Entity* e, BoundingBox& boundingBox, float& dt) {
	MovementComponent* movement = e->getComponent<MovementComponent>();
	TransformComponent* transform = e->getComponent<TransformComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();

	const float velocityAmp = glm::length(movement->velocity) * dt;

	//Ray cast to find upcoming collisions, use padding for "swept sphere"
	Octree::RayIntersectionInfo intersectionInfo;
	m_octree->getRayIntersection(boundingBox.getPosition(), glm::normalize(movement->velocity), &intersectionInfo, e, collision->padding, collision->doSimpleCollisions);

	float closestHit = intersectionInfo.closestHit + 0.01f; //Force the projectile to move forward atleast a little bit to avoid getting stuck in endless loops

	if (closestHit <= velocityAmp && closestHit >= 0.0f) { //Found upcoming collision

		//Calculate new dt
		float newDt = ((closestHit) / velocityAmp) * dt;

		//Move untill first overlap
		boundingBox.setPosition(boundingBox.getPosition() + movement->velocity * newDt);
		transform->translate(movement->velocity * newDt);

		dt -= newDt;

		//Collision update
		if (handleCollisions(e, intersectionInfo.info, 0.0f)) {
			surfaceFromCollision(e, intersectionInfo.info);
		}

		rayCastUpdate(e, boundingBox, dt);
	}
}

void CollisionSystem::surfaceFromCollision(Entity* e, std::vector<Octree::CollisionInfo>& collisions) {
	glm::vec3 distance(0.0f);
	BoundingBox*          bb        = e->getComponent<BoundingBoxComponent>()->getBoundingBox();
	TransformComponent*   transform = e->getComponent<TransformComponent>();

	const size_t count = collisions.size();
	for (size_t i = 0; i < count; i++) {
		const Octree::CollisionInfo& collisionInfo_i = collisions[i];
		float depth;
		glm::vec3 axis;

		if (collisionInfo_i.shape->getIntersectionDepthAndAxis(bb, &axis, &depth)) {
			bb->setPosition(bb->getPosition() + axis * (depth - 0.0001f));
			distance += axis * (depth - 0.0001f);
		}
	}
	transform->translate(distance);
}