#pragma once

#include "../../BaseComponentSystem.h"
#include "Sail/ai/pathfinding/NodeSystem.h"

class TransformComponent;
class AiComponent;
class GunComponent;
class MovementComponent;
class SpeedLimitComponent;
class NodeSystem;
class Model;
class Octree;
class MapComponent;

#ifdef _DEBUG_NODESYSTEM
class Shader;
#endif

class AiSystem final : public BaseComponentSystem {
public:
	AiSystem();
	~AiSystem();

	void initNodeSystem(Octree* octree);

	/*
		Adds an entity to the system
	*/
	//bool addEntity(Entity* entity) override;

	std::vector<Entity*>& getEntities();

	void update(float dt) override;

	NodeSystem* getNodeSystem();

	virtual void stop() override;

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
	const float getAveragePathSearchTime() const;
	const float getAverageAiUpdateTime() const;
#endif

private:
	void updatePath(Entity* e);
	void updatePhysics(Entity* e, float dt);
	float getAiYaw(MovementComponent* moveComp, float currYaw, float dt);
	void aiUpdateFunc(Entity* e, const float dt);
	glm::vec3 getDesiredDir(AiComponent* aiComp, TransformComponent* transComp);
	bool nodeConnectionCheck(glm::vec3 nodePos, glm::vec3 otherNodePos, Entity* nodeEnt);
	glm::vec3 getNodePos(const int x, const int z, float nodeSize, float nodePadding, float startOffsetX, float startOffsetZ);

private:
	float m_timeBetweenPathUpdate;

	std::unique_ptr<NodeSystem> m_nodeSystem;

	Octree* m_octree;

	const static unsigned int NUM_UPDATE_TIMES = 10;
	float m_updateTimes[NUM_UPDATE_TIMES];
	unsigned int m_currUpdateTimeIndex = 0;
};