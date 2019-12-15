#pragma once

#include "Sail/ai/states/State.h"

class NodeSystem;
class RendererWrapper;

class WaterSearchingState : public FSM::State<WaterSearchingState> {
public:
	WaterSearchingState(NodeSystem* nodeSystem);
	~WaterSearchingState();

	void update(float dt, Entity* entity) override;
	void reset(Entity* entity) override;
	void init(Entity* entity) override;

	bool* getDoSwitch();

private:
	void findRandomNodeIndex(const int currNodeIndex, int offsetX = 1, int offsetZ = 1);
	bool searchForWater(const glm::vec3& currPos, const int currNodeIndex);

	NodeSystem* m_nodeSystemRef;
	RendererWrapper* m_rendererWrapperRef;

	glm::vec3 m_targetPos;
	float m_searchingClock;

	bool m_doSwitch;
};
