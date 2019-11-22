#include "pch.h"
#include "CollisionSystem.h"
#include "..//..//components/TransformComponent.h"
#include "..//..//components/MovementComponent.h"
#include "..//..//components/CollisionComponent.h"
#include "..//..//components/BoundingBoxComponent.h"
#include "..//..//components/RagdollComponent.h"
#include "..//..//Physics/Intersection.h"

#include "Sail/Application.h"

CollisionSystem::CollisionSystem() {
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<CollisionComponent>(true, true, true);
	registerComponent<BoundingBoxComponent>(true, true, true);
	registerComponent<RagdollComponent>(false, true, false);

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

#ifdef DEVELOPMENT
unsigned int CollisionSystem::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif

bool CollisionSystem::collisionUpdatePart(float dt, size_t start, size_t end) {
	for (size_t i = start; i < end; ++i) {
		Entity* e = entities[i];

		CollisionComponent* collision = e->getComponent<CollisionComponent>();
		BoundingBoxComponent* boundingBox = e->getComponent<BoundingBoxComponent>();

		collision->collisions.clear();

		if (collision->padding < 0.0f) {
			glm::vec3 halfSize = boundingBox->getBoundingBox()->getHalfSize();
			collision->padding = glm::min(glm::min(halfSize.x, halfSize.y), halfSize.z);
		}

		collisionUpdate(e, dt);
	}

	return true;
}

bool CollisionSystem::surfaceFromCollisionPart(float dt, size_t start, size_t end) {
	for (size_t i = start; i < end; ++i) {
		Entity* e = entities[i];

		CollisionComponent* collision = e->getComponent<CollisionComponent>();

		if (m_octree) {
			if (!e->hasComponent<RagdollComponent>()) {
				surfaceFromCollision(e, e->getComponent<BoundingBoxComponent>()->getBoundingBox(), collision->collisions);
			}
			else {
				surfaceFromRagdollCollision(e, collision->collisions);
			}
		}
	}
	return true;
}

bool CollisionSystem::rayCastCollisionPart(float dt, size_t start, size_t end) {
	for (size_t i = start; i < end; ++i) {
		Entity* e = entities[i];

		MovementComponent* movement = e->getComponent<MovementComponent>();
		BoundingBox* boundingBox = e->getComponent<BoundingBoxComponent>()->getBoundingBox();

		float updateableDt = dt;

		if (m_octree) {
			if (!e->hasComponent<RagdollComponent>()) {
				if (rayCastCheck(e, boundingBox, updateableDt)) {
					//Object is moving fast, ray cast for collisions
					rayCastUpdate(e, boundingBox, updateableDt);
					movement->oldVelocity = movement->velocity;
				}
			}
			else {
				RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();
				bool rayCastingNeeded = false;

				for (size_t j = 0; j < ragdollComp->contactPoints.size(); j++) {
					if (rayCastCheck(e, &ragdollComp->contactPoints[j].boundingBox, dt)) {
						rayCastingNeeded = true;
						break;
					}
				}

				if (rayCastingNeeded) {
					//Object is moving fast, ray cast for collisions
					rayCastRagdollUpdate(e, updateableDt);
					movement->oldVelocity = movement->velocity;
				}
			}
		}
		movement->updateableDt = updateableDt;
	}
	return true;
}

void CollisionSystem::collisionUpdate(Entity* e, const float dt) {
	//Update collision data
	CollisionComponent* collision = e->getComponent<CollisionComponent>();
	std::vector<Octree::CollisionInfo> collisions;

	if (!e->hasComponent<RagdollComponent>()) {
		m_octree->getCollisions(e, e->getComponent<BoundingBoxComponent>()->getBoundingBox(), &collisions, collision->doSimpleCollisions);
		handleCollisions(e, collisions, dt);
	}
	else {
		RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();

		for (size_t i = 0; i < ragdollComp->contactPoints.size(); i++) {
			m_octree->getCollisions(e, &ragdollComp->contactPoints[i].boundingBox, &collisions, collision->doSimpleCollisions);
		}

		handleRagdollCollisions(e, collisions, dt);
	}
}


// Modifies Entity e's Movement- and CollisionComponent
// Neither of which is used in the rest of updatePart() so modifying them should be fine
const bool CollisionSystem::handleCollisions(Entity* e, std::vector<Octree::CollisionInfo>& collisions, const float dt) {
	MovementComponent* movement = e->getComponent<MovementComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();
	const BoundingBox* boundingBox = e->getComponent<BoundingBoxComponent>()->getBoundingBox();

	bool collisionFound = false;
	collision->onGround = false;

	const size_t collisionCount = collisions.size();

	if (collisionCount > 0) {
		std::vector<int> groundIndices;
		glm::vec3 sumVec(0.0f);
		std::vector<Octree::CollisionInfo> trueCollisions;

		//Gather info
		gatherCollisionInformation(e, boundingBox, collisions, trueCollisions, sumVec, groundIndices, dt);

		if (trueCollisions.size() > 0) {
			collisionFound = true;
		}

		if (groundIndices.size() > 0) {
			collision->onGround = true;
		}

		//Handle true collisions
		updateVelocityVec(e, movement->velocity, trueCollisions, sumVec, groundIndices, dt);
	}

	return collisionFound;
}

const bool CollisionSystem::handleRagdollCollisions(Entity* e, std::vector<Octree::CollisionInfo>& collisions, const float dt) {
	MovementComponent* movementComp = e->getComponent<MovementComponent>();
	TransformComponent* transComp = e->getComponent<TransformComponent>();
	RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();

	bool collisionFound = false;
	collision->onGround = false;

	std::vector<int> groundIndices;
	glm::vec3 sumVec(0.0f);
	std::vector<Octree::CollisionInfo> trueCollisions;

	for (size_t i = 0; i < ragdollComp->contactPoints.size(); i++) {
		//----WILL BE USED LATER ON WHEN MOMENTUM IS IMPLEMENTED----
		/*Transform tempTransform;
		tempTransform.setRotations(transComp->getRotations());
		glm::vec3 pos1 = tempTransform.getMatrixWithUpdate() * glm::vec4(ragdollComp->contactPoints[i].localOffSet, 1.0f);
		tempTransform.rotate(movementComp->rotation * dt);
		glm::vec3 pos2 = tempTransform.getMatrixWithUpdate() * glm::vec4(ragdollComp->contactPoints[i].localOffSet, 1.0f);

		glm::vec3 bbMovement(0.f);
		glm::vec3 bbDir = pos2 - pos1;
		if (glm::length2(bbDir) > 0.0000001f) {
			bbDir = glm::normalize(bbDir);
			float bbVelocity = (glm::length(movementComp->rotation) / 2.0f * glm::pi<float>()) * glm::length(ragdollComp->contactPoints[i].localOffSet) * 2.0f * glm::pi<float>();
			glm::vec3 bbMovement = bbDir * bbVelocity;
		}

		bbMovement += movementComp->velocity;
		glm::vec3 updatedMovement = bbMovement; */
		//----------------------------------------------------------

		//Gather info
		gatherCollisionInformation(e, &ragdollComp->contactPoints[i].boundingBox, collisions, trueCollisions, sumVec, groundIndices, dt);
	}

	if (trueCollisions.size() > 0) {
		collisionFound = true;
	}

	if (groundIndices.size() > 0) {
		collision->onGround = true;
	}


	glm::vec3 movement = movementComp->velocity;

	//Update velocity
	updateVelocityVec(e, movement, trueCollisions, sumVec, groundIndices, dt);

	movementComp->velocity = movement;

	return collisionFound;
}

void CollisionSystem::gatherCollisionInformation(Entity* e, const BoundingBox* boundingBox, std::vector<Octree::CollisionInfo>& collisions, std::vector<Octree::CollisionInfo>& trueCollisions, glm::vec3& sumVec, std::vector<int>& groundIndices, const float dt) {
	CollisionComponent* collision = e->getComponent<CollisionComponent>();
	
	const size_t collisionCount = collisions.size();

	if (collisionCount > 0) {
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

				//Save ground collisions
				if (collisionInfo_i.intersectionAxis.y > 0.7f) {
					bool newGround = true;
					for (size_t j = 0; j < groundIndices.size(); j++) {
						if (collisionInfo_i.intersectionAxis == trueCollisions[groundIndices[j]].intersectionAxis) {
							newGround = false;
						}
					}
					if (newGround) {
						//Save collision for friction calculation
						groundIndices.push_back((int)trueCollisions.size() - 1);
					}
				}
			}
		}
	}
}

void CollisionSystem::updateVelocityVec(Entity* e, glm::vec3& velocity, std::vector<Octree::CollisionInfo>& collisions, glm::vec3& sumVec, std::vector<int>& groundIndices, const float dt) {
	CollisionComponent* collision = e->getComponent<CollisionComponent>();

	const size_t collisionCount = collisions.size();

	//Loop through collisions and handle them
	for (size_t i = 0; i < collisionCount; i++) {
		const Octree::CollisionInfo& collisionInfo_i = collisions[i];

		//----Velocity changes from collisions----

		//Stop movement towards triangle
		float projectionSize = glm::dot(velocity, -collisionInfo_i.intersectionAxis);

		if (projectionSize > 0.0f) { //Is pushing against wall
			velocity += collisionInfo_i.intersectionAxis * (projectionSize * (1.0f + collision->bounciness)); //Limit movement towards wall
		}


		//Tight angle corner special case
		const float dotProduct = glm::dot(collisionInfo_i.intersectionAxis, glm::normalize(sumVec));
		if (dotProduct < 0.7072f && dotProduct > 0.0f) { //Colliding in a tight angle corner
			glm::vec3 normalToNormal = sumVec - glm::dot(sumVec, collisionInfo_i.intersectionAxis) * collisionInfo_i.intersectionAxis;
			normalToNormal = glm::normalize(normalToNormal);

			//Stop movement towards corner
			projectionSize = glm::dot(velocity, -normalToNormal);

			if (projectionSize > 0.0f) {
				velocity += normalToNormal * projectionSize * (1.0f + collision->bounciness);
			}
		}
		//----------------------------------------
	}

	//----Drag----
	if (collision->onGround) { //Ground drag
		size_t nrOfGroundCollisions = groundIndices.size();
		for (size_t i = 0; i < nrOfGroundCollisions; i++) {
			const Octree::CollisionInfo& collisionInfo_ground_i = collisions[groundIndices[i]];
			const glm::vec3 velAlongPlane = velocity - collisionInfo_ground_i.intersectionAxis * glm::dot(collisionInfo_ground_i.intersectionAxis, velocity);
			const float sizeOfVel = glm::length(velAlongPlane);
			if (sizeOfVel > 0.0f) {
				const float slowdown = glm::min((collision->drag / nrOfGroundCollisions) * dt, sizeOfVel);
				velocity -= slowdown * glm::normalize(velAlongPlane);
			}
		}
	}
	//------------
}

const bool CollisionSystem::rayCastCheck(Entity* e, const BoundingBox* boundingBox, float& dt) const {
	const MovementComponent* movementComp = e->getComponent<MovementComponent>();

	if (glm::abs(movementComp->velocity.x * dt) > glm::abs(boundingBox->getHalfSize().x)
		|| glm::abs(movementComp->velocity.y * dt) > glm::abs(boundingBox->getHalfSize().y)
		|| glm::abs(movementComp->velocity.z * dt) > glm::abs(boundingBox->getHalfSize().z)) {
		//Object is moving at a speed that risks missing collisions
		return true;
	}
	return false;
}

void CollisionSystem::rayCastUpdate(Entity* e, BoundingBox* boundingBox, float& dt) {
	MovementComponent* movement = e->getComponent<MovementComponent>();
	TransformComponent* transform = e->getComponent<TransformComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();

	const float velocityAmp = glm::length(movement->velocity) * dt;

	//Ray cast to find upcoming collisions, use padding for "swept sphere"
	Octree::RayIntersectionInfo intersectionInfo;
	m_octree->getRayIntersection(boundingBox->getPosition(), glm::normalize(movement->velocity), &intersectionInfo, e, collision->padding, collision->doSimpleCollisions);

	float closestHit = intersectionInfo.closestHit + 0.01f; //Force small forwards movement to avoid getting stuck in infinite loops

	if (closestHit <= velocityAmp && closestHit >= 0.0f) { //Found upcoming collision

		//Calculate new dt
		float newDt = ((closestHit) / velocityAmp) * dt;

		//Move untill first overlap
		boundingBox->setPosition(boundingBox->getPosition() + movement->velocity * newDt);
		transform->translate(movement->velocity * newDt);

		dt -= newDt;

		//Collision update
		if (handleCollisions(e, intersectionInfo.info, 0.0f)) {
			surfaceFromCollision(e, boundingBox, intersectionInfo.info);
		}

		rayCastUpdate(e, boundingBox, dt);
	}
}

void CollisionSystem::rayCastRagdollUpdate(Entity* e, float& dt) {
	MovementComponent* movement = e->getComponent<MovementComponent>();
	TransformComponent* transform = e->getComponent<TransformComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();
	RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();

	const float velocityAmp = glm::length(movement->velocity) * dt;

	float closestHit = INFINITY;

	std::vector<Octree::CollisionInfo> collisions;

	for (size_t i = 0; i < ragdollComp->contactPoints.size(); i++) {
		//Ray cast to find upcoming collisions, use padding for "swept sphere"
		Octree::RayIntersectionInfo intersectionInfo;
		float padding = glm::min(glm::min(ragdollComp->contactPoints[i].boundingBox.getHalfSize().x, ragdollComp->contactPoints[i].boundingBox.getHalfSize().y), ragdollComp->contactPoints[i].boundingBox.getHalfSize().z);
		m_octree->getRayIntersection(ragdollComp->contactPoints[i].boundingBox.getPosition(), glm::normalize(movement->velocity), &intersectionInfo, e, padding, collision->doSimpleCollisions);
		if (intersectionInfo.closestHit >= 0.f) {
			if (intersectionInfo.closestHit < closestHit) {
				closestHit = intersectionInfo.closestHit;
			}

			collisions.push_back(intersectionInfo.info[intersectionInfo.closestHitIndex]);
		}
		
	}

	closestHit += 0.01f; //Force small forwards movement to avoid getting stuck in infinite loops

	if (closestHit <= velocityAmp && closestHit >= 0.0f) { //Found upcoming collision

		//Calculate new dt
		float newDt = ((closestHit) / velocityAmp) * dt;

		//Move untill first overlap
		for (size_t i = 0; i < ragdollComp->contactPoints.size(); i++) {
			ragdollComp->contactPoints[i].boundingBox.setPosition(ragdollComp->contactPoints[i].boundingBox.getPosition() + movement->velocity * newDt);
		}

		transform->translate(movement->velocity* newDt);

		dt -= newDt;

		//Collision update
		if (handleRagdollCollisions(e, collisions, 0.0f)) {
			surfaceFromRagdollCollision(e, collisions);
		}

		rayCastRagdollUpdate(e, dt);
	}
}

glm::vec3 CollisionSystem::surfaceFromCollision(Entity* e, BoundingBox* boundingBox, std::vector<Octree::CollisionInfo>& collisions) {
	glm::vec3 distance(0.0f);
	TransformComponent* transform = e->getComponent<TransformComponent>();

	const size_t count = collisions.size();
	for (size_t i = 0; i < count; i++) {
		const Octree::CollisionInfo& collisionInfo_i = collisions[i];
		float depth;
		glm::vec3 axis;

		if (collisionInfo_i.shape->getIntersectionDepthAndAxis(boundingBox, &axis, &depth)) {
			boundingBox->setPosition(boundingBox->getPosition() + axis * (depth - 0.0001f));
			distance += axis * (depth - 0.0001f);
		}
	}
	transform->translate(distance);

	return distance;
}

void CollisionSystem::surfaceFromRagdollCollision(Entity* e, std::vector<Octree::CollisionInfo>& collisions) {
	RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();
	for (size_t i = 0; i < ragdollComp->contactPoints.size(); i++) {
		glm::vec3 distance = surfaceFromCollision(e, &ragdollComp->contactPoints[i].boundingBox, collisions);

		for (size_t j = 0; j < ragdollComp->contactPoints.size(); j++) {
			if (j != i) {
				ragdollComp->contactPoints[j].boundingBox.setPosition(ragdollComp->contactPoints[j].boundingBox.getPosition() + distance);
			}
		}
	}
}
