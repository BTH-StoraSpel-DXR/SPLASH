#include "pch.h"
#include "CollisionSystem.h"
#include "..//..//components/TransformComponent.h"
#include "..//..//components/MovementComponent.h"
#include "..//..//components/CollisionComponent.h"
#include "..//..//components/BoundingBoxComponent.h"
#include "..//..//components/RagdollComponent.h"
#include "..//..//components/RenderInActiveGameComponent.h"
#include "..//..//components/RenderInReplayComponent.h"
#include "..//..//Physics/Intersection.h"

#include "Sail/Application.h"

template <typename T>
CollisionSystem<T>::CollisionSystem() {
	registerComponent<MovementComponent>(true, true, true);
	registerComponent<TransformComponent>(true, true, true);
	registerComponent<CollisionComponent>(true, true, true);
	registerComponent<BoundingBoxComponent>(true, true, true);
	registerComponent<RagdollComponent>(false, true, false);
	registerComponent<T>(true, false, false);

	m_octree = nullptr;
}

template <typename T>
CollisionSystem<T>::~CollisionSystem() {
}

template <typename T>
void CollisionSystem<T>::provideOctree(Octree* octree) {
	m_octree = octree;
}

template <typename T>
void CollisionSystem<T>::update(float dt) {
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
template <typename T>
unsigned int CollisionSystem<T>::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif

template <typename T>
bool CollisionSystem<T>::collisionUpdatePart(float dt, size_t start, size_t end) {
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

template <typename T>
bool CollisionSystem<T>::surfaceFromCollisionPart(float dt, size_t start, size_t end) {
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

template <typename T>
bool CollisionSystem<T>::rayCastCollisionPart(float dt, size_t start, size_t end) {
	for (size_t i = start; i < end; ++i) {
		Entity* e = entities[i];

		MovementComponent* movement = e->getComponent<MovementComponent>();
		BoundingBox* boundingBox = e->getComponent<BoundingBoxComponent>()->getBoundingBox();
		TransformComponent* transComp = e->getComponent<TransformComponent>();

		float updateableDt = dt;

		if (m_octree) {
			if (!e->hasComponent<RagdollComponent>()) {
				if (rayCastCheck(e, boundingBox, movement->velocity, updateableDt)) {
					//Object is moving fast, ray cast for collisions
					rayCastUpdate(e, boundingBox, updateableDt);
					movement->oldVelocity = movement->velocity;
				}
			}
			else {
				RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();
				bool rayCastingNeeded = false;

				for (size_t j = 0; j < ragdollComp->contactPoints.size(); j++) {
					if (rayCastCheck(e, &ragdollComp->contactPoints[j].boundingBox, movement->velocity, dt)) {
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

template <typename T>
void CollisionSystem<T>::collisionUpdate(Entity* e, const float dt) {
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

		handleRagdollCollisions(e, collisions, true, dt);
	}
}


// Modifies Entity e's Movement- and CollisionComponent
// Neither of which is used in the rest of updatePart() so modifying them should be fine
template <typename T>
const bool CollisionSystem<T>::handleCollisions(Entity* e, std::vector<Octree::CollisionInfo>& collisions, const float dt) {
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

template <typename T>
const bool CollisionSystem<T>::handleRagdollCollisions(Entity* e, std::vector<Octree::CollisionInfo>& collisions, bool calculateMomentum, const float dt) {
	MovementComponent* movementComp = e->getComponent<MovementComponent>();
	TransformComponent* transComp = e->getComponent<TransformComponent>();
	RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();

	bool collisionFound = false;
	collision->onGround = false;

	std::vector<glm::vec3> movementDiffs;

	for (size_t i = 0; i < ragdollComp->contactPoints.size(); i++) {
		//----Avoid spinning into things----
		glm::vec3 globalCenterOfMass = transComp->getMatrixWithUpdate() * glm::vec4(ragdollComp->localCenterOfMass, 1.0f);
		glm::vec3 offsetVector = transComp->getMatrixWithUpdate() * glm::vec4(ragdollComp->contactPoints[i].localOffSet, 1.0f) - glm::vec4(globalCenterOfMass, 1.0f);
		if (glm::length2(offsetVector) > 0.f) {
			Octree::RayIntersectionInfo tempInfo;
			m_octree->getRayIntersection(globalCenterOfMass, glm::normalize(offsetVector), &tempInfo, e, 0.0f, collision->doSimpleCollisions);

			if (tempInfo.closestHit >= 0.0f && tempInfo.closestHit < glm::length(offsetVector)) {
				glm::vec3 translation = (tempInfo.closestHit - glm::length(offsetVector)) * 1.1f * glm::normalize(offsetVector);
				transComp->translate(translation);
				for (size_t j = 0; j < ragdollComp->contactPoints.size(); j++) {
					ragdollComp->contactPoints[j].boundingBox.setPosition(ragdollComp->contactPoints[j].boundingBox.getPosition() + translation);
				}
			}
		}
		//----------------------------------

		std::vector<int> groundIndices;
		glm::vec3 sumVec(0.0f);
		std::vector<Octree::CollisionInfo> trueCollisions;

		//Gather info
		gatherCollisionInformation(e, &ragdollComp->contactPoints[i].boundingBox, collisions, trueCollisions, sumVec, groundIndices, dt);

		if (groundIndices.size() > 0) {
			collision->onGround = true;
		}

		movementDiffs.emplace_back();
		movementDiffs.back() = { 0.f, 0.f, 0.f };

		glm::vec3 bbMovement(0.f);
		bbMovement += getAngularVelocity(e, ragdollComp->contactPoints[i].localOffSet, ragdollComp->localCenterOfMass);
		bbMovement += movementComp->velocity;

		if (trueCollisions.size() > 0) {
			collisionFound = true;

			glm::vec3 updatedMovement = bbMovement;

			//Update velocity
			updateVelocityVec(e, updatedMovement, trueCollisions, sumVec, groundIndices, dt);

			movementDiffs.back() = updatedMovement - bbMovement;
		}
	}

	glm::vec3 totalMovementDiff(0.f);
	glm::vec3 totalRotation(0.f);
	int movCounter = 0;

	for (size_t i = 0; i < ragdollComp->contactPoints.size(); i++) {
		movCounter++;
		glm::vec3 offsetVector = transComp->getMatrixWithUpdate() * glm::vec4(ragdollComp->contactPoints[i].localOffSet, 1.0f) - transComp->getMatrixWithUpdate() * glm::vec4(ragdollComp->localCenterOfMass, 1.0f);
		if (glm::length2(movementDiffs[i]) > 0.001f && glm::length2(offsetVector) > 0.001f) {
			//float hitDot = glm::max(glm::dot(glm::normalize(movementDiffs[i]), glm::normalize(-offsetVector)), 0.f);
			totalMovementDiff += movementDiffs[i];// * hitDot;

			if (calculateMomentum) {
				glm::vec3 rotationVec = glm::normalize(glm::cross(offsetVector, movementDiffs[i]));
				glm::vec3 movementVec = glm::normalize(glm::cross(offsetVector, rotationVec));

				movementVec = movementVec * glm::dot(movementDiffs[i], movementVec);

				float angle = glm::atan(glm::length(movementVec), glm::length(offsetVector));
				totalRotation += rotationVec * glm::pow(angle, 0.6f) * 2.5f;
			}
		}
	}

	movementComp->velocity += totalMovementDiff / glm::max((float)movCounter, 1.0f);
	if (glm::length2(totalRotation) > 0.0001f) {
		movementComp->rotation = totalRotation;
	}

	return collisionFound;
}

template <typename T>
void CollisionSystem<T>::gatherCollisionInformation(Entity* e, const BoundingBox* boundingBox, std::vector<Octree::CollisionInfo>& collisions, std::vector<Octree::CollisionInfo>& trueCollisions, glm::vec3& sumVec, std::vector<int>& groundIndices, const float dt) {
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

template <typename T>
void CollisionSystem<T>::updateVelocityVec(Entity* e, glm::vec3& velocity, std::vector<Octree::CollisionInfo>& collisions, glm::vec3& sumVec, std::vector<int>& groundIndices, const float dt) {
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

template <typename T>
const bool CollisionSystem<T>::rayCastCheck(Entity* e, const BoundingBox* boundingBox, const glm::vec3& velocity, const float& dt) const {
	if (glm::abs(velocity.x * dt) > glm::abs(boundingBox->getHalfSize().x)
		|| glm::abs(velocity.y * dt) > glm::abs(boundingBox->getHalfSize().y)
		|| glm::abs(velocity.z * dt) > glm::abs(boundingBox->getHalfSize().z)) {
		//Object is moving at a speed that risks missing collisions
		return true;
	}
	return false;
}

template <typename T>
void CollisionSystem<T>::rayCastUpdate(Entity* e, BoundingBox* boundingBox, float& dt) {
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

template <typename T>
void CollisionSystem<T>::rayCastRagdollUpdate(Entity* e, float& dt) {
	MovementComponent* movement = e->getComponent<MovementComponent>();
	TransformComponent* transform = e->getComponent<TransformComponent>();
	CollisionComponent* collision = e->getComponent<CollisionComponent>();
	RagdollComponent* ragdollComp = e->getComponent<RagdollComponent>();

	const float velocityAmp = glm::length(movement->velocity) * dt;

	float closestHit = 9999999.0f;

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

		transform->translate(movement->velocity * newDt);

		dt -= newDt;

		//Collision update
		if (handleRagdollCollisions(e, collisions, true, 0.0f)) {
			surfaceFromRagdollCollision(e, collisions);
		}

		rayCastRagdollUpdate(e, dt);
	}
}

template <typename T>
glm::vec3 CollisionSystem<T>::surfaceFromCollision(Entity* e, BoundingBox* boundingBox, std::vector<Octree::CollisionInfo>& collisions) {
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

template <typename T>
void CollisionSystem<T>::surfaceFromRagdollCollision(Entity* e, std::vector<Octree::CollisionInfo>& collisions) {
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

template <typename T>
glm::vec3 CollisionSystem<T>::getAngularVelocity(Entity* e, const glm::vec3& offset, const glm::vec3& centerOfMass) {
	TransformComponent* transComp = e->getComponent<TransformComponent>();
	MovementComponent* movementComp = e->getComponent<MovementComponent>();

	//----Angular momentum----
	glm::vec3 bbMovement(0.f);
	glm::vec3 offsetVector = transComp->getMatrixWithUpdate() * glm::vec4(offset, 1.0f) - transComp->getMatrixWithUpdate() * glm::vec4(centerOfMass, 1.0f);

	glm::vec3 bbDir = glm::cross(offsetVector, movementComp->rotation);
	if (glm::length2(bbDir) > 0.0001f) {
		bbDir = glm::normalize(bbDir);
		float bbVelocity = (glm::length(movementComp->rotation) / 2.0f * glm::pi<float>()) * glm::length(offsetVector) * 2.0f * glm::pi<float>();
		glm::vec3 bbMovement = bbDir * bbVelocity;
	}
	//------------------------

	return bbMovement;
}


template class CollisionSystem<RenderInActiveGameComponent>;
template class CollisionSystem<RenderInReplayComponent>;