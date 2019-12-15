#pragma once
#include "..//BaseComponentSystem.h"
#include "..//..//Physics/Octree.h"
#include "..//..//Physics/BoundingBox.h"

template <typename T>
class CollisionSystem final : public BaseComponentSystem {
public:
	CollisionSystem();
	~CollisionSystem();
	
	void provideOctree(Octree* octree);
	void update(float dt);

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
#endif

private:
	bool collisionUpdatePart(float dt, size_t start, size_t end);
	bool surfaceFromCollisionPart(float dt, size_t start, size_t end);
	bool rayCastCollisionPart(float dt, size_t start, size_t end);
	
	const bool rayCastCheck(Entity* e, const BoundingBox* boundingBox, const glm::vec3& velocity, const float& dt) const;
	void rayCastUpdate(Entity* e, BoundingBox* boundingBox, float& dt);
	void rayCastRagdollUpdate(Entity* e, float& dt);
	void collisionUpdate(Entity* e, const float dt);
	const bool handleCollisions(Entity* e, std::vector<Octree::CollisionInfo>& collisions, const float dt);
	const bool handleRagdollCollisions(Entity* e, std::vector<Octree::CollisionInfo>& collisions, bool calculateMomentum, const float dt);
	void gatherCollisionInformation(Entity* e, const BoundingBox* boundingBox, std::vector<Octree::CollisionInfo>& collisions, std::vector<Octree::CollisionInfo>& trueCollisions, glm::vec3& sumVec, std::vector<int>& groundIndices, const float dt);
	void updateVelocityVec(Entity* e, glm::vec3& velocity, std::vector<Octree::CollisionInfo>& collisions, glm::vec3& sumVec, std::vector<int>& groundIndices, const float dt);
	glm::vec3 surfaceFromCollision(Entity* e, BoundingBox* boundingBox, std::vector<Octree::CollisionInfo>& collisions);
	void surfaceFromRagdollCollision(Entity* e, std::vector<Octree::CollisionInfo>& collisions);

	glm::vec3 getAngularVelocity(Entity* e, const glm::vec3& offset, const glm::vec3& centerOfMass);

private:
	Octree* m_octree;
};