#pragma once

#include "BoundingBox.h"
#include "Sphere.h"
#include "CollisionShapes.h"
#include "Sail/entities/Entity.h"

class Model;
class Camera;
struct Frustum;

class Octree {
public:
	class CollisionInfo {
	public:
		CollisionInfo() : intersectionAxis(glm::vec3(0.0f)), intersectionPosition(glm::vec3(0.0f)), shape(nullptr), entity(nullptr) {};
		~CollisionInfo() {
			if (shape != nullptr) {
				shape->keeperTracker--;
				if (shape->keeperTracker == 0) {
					delete shape;
				}
			}
		};


		CollisionInfo(const CollisionInfo& other) {
			this->intersectionAxis = other.intersectionAxis;
			this->intersectionPosition = other.intersectionPosition;
			other.shape->keeperTracker++;
			this->shape = other.shape;
			this->entity = other.entity;
		}
		CollisionInfo& operator=(const CollisionInfo& other) {
			this->intersectionAxis = other.intersectionAxis;
			this->intersectionPosition = other.intersectionPosition;
			other.shape->keeperTracker++;
			this->shape = other.shape;
			this->entity = other.entity;
			return *this;
		}

		glm::vec3 intersectionAxis;
		glm::vec3 intersectionPosition;
		CollisionShape* shape;
		Entity* entity;
	};

	struct RayIntersectionInfo {
		float closestHit = -1.0f;
		int closestHitIndex = -1;
		std::vector<Octree::CollisionInfo> info;
	};

private:
	struct Node {
		std::vector<Node> childNodes;
		Node* parentNode = nullptr;
		Entity::SPtr bbEntity;
		int nrOfEntities = 0;
		std::vector<Entity*> entities;
	};

	Node m_baseNode;

	Model* m_boundingBoxModel;

	int m_softLimitMeshes;
	float m_minimumNodeHalfSize;

	void expandBaseNode(glm::vec3 direction);
	glm::vec3 findCornerOutside(Entity* entity, Node* testNode);
	bool addEntityRec(Entity* newEntity, Node* currentNode);
	bool removeEntityRec(Entity* entityToRemove, Node* currentNode);
	void updateRec(Node* currentNode, std::vector<Entity*>* entitiesToReAdd);
	void getCollisionData(const BoundingBox* entityBoundingBox, Entity* meshEntity, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, std::vector<Octree::CollisionInfo>* outCollisionData);
	void getCollisionsRec(Entity* entity, const BoundingBox* entityBoundingBox, Node* currentNode, std::vector<Octree::CollisionInfo>* outCollisionData, const bool doSimpleCollisions);
	void getIntersectionData(const glm::vec3& rayStart, const glm::vec3& rayDir, Entity* meshEntity, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, RayIntersectionInfo* outIntersectionData, float padding);
	void getRayIntersectionRec(const glm::vec3& rayStart, const glm::vec3& rayDir, Node* currentNode, RayIntersectionInfo* outIntersectionData, Entity* ignoreThis, float padding, const bool doSimpleIntersections);
	int pruneTreeRec(Node* currentNode);
	int frustumCulledDrawRec(const Frustum& frustum, Node* currentNode);

public:
	Octree(Model *boundingBoxModel);
	~Octree();

	void addEntity(Entity* newEntity);
	void addEntities(std::vector<Entity*> *newEntities);

	void removeEntity(Entity* entityToRemove);
	void removeEntities(std::vector<Entity*> entitiesToRemove);

	void update();

	void getCollisions(Entity* entity, const BoundingBox* entityBoundingBox, std::vector<CollisionInfo>* outCollisionData, const bool doSimpleCollisions = false);
	void getRayIntersection(const glm::vec3& rayStart, const glm::vec3& rayDir, RayIntersectionInfo* outIntersectionData, Entity* ignoreThis = nullptr, float padding = 0.0f, const bool doSimpleIntersections = false);

	int frustumCulledDraw(Camera& camera);
};