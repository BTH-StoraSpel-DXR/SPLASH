#pragma once

#include "Sail/ai/states/State.h"

class NodeSystem;

class SearchingState : public FSM::State<SearchingState> {
public:
	SearchingState(NodeSystem* nodeSystem);
	~SearchingState();

	void update(float dt, Entity* entity) override;
	void reset(Entity* entity) override;
	void init(Entity* entity) override;
	float* getDistToHost();

private:

	void findRandomNodeIndex(int currNodeIndex);

	NodeSystem* m_nodeSystem;

	// This needs to be changed when playing with multiple players / bots
	float m_distToHost;
	int m_targetNode;

	float m_searchingClock;
};
