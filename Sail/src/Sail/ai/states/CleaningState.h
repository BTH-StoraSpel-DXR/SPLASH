#pragma once

#include "Sail/ai/states/State.h"

class NodeSystem;
class RendererWrapper;

class CleaningState : public FSM::State<CleaningState> {
public:
	CleaningState(NodeSystem* nodeSystem);
	~CleaningState(); 

	void update(float dt, Entity* entity) override;
	void reset(Entity* entity) override;
	void init(Entity* entity) override;

	bool* getDoSwitch();

	void createCleaningPath(Entity* entity);

private:
	NodeSystem* m_nodeSystemRef;
	RendererWrapper* m_rendererWrapperRef;
	//int m_targetNode;
	glm::vec3 m_targetPos;
	float m_searchingClock;

	bool m_doSwitch;

	float m_maxStateTime;
	
	unsigned int m_cleaningPathStart;
};
