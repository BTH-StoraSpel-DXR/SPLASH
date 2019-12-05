#include "pch.h"
#include "SearchingState.h"

#include "Sail/ai/pathfinding/NodeSystem.h"
#include "Sail/entities/components/AiComponent.h"
#include "Sail/entities/components/TransformComponent.h"

#include "Sail/utils/Utils.h"

SearchingState::SearchingState(NodeSystem* nodeSystem) : m_nodeSystem(nodeSystem) {
	m_distToHost = 9999999.0f;
	m_targetNode = 0;
	m_searchingClock = 4.f;
}

SearchingState::~SearchingState() {
}

void SearchingState::update(float dt, Entity* entity) {
	
	auto aiComp = entity->getComponent<AiComponent>();
	auto aiPos = entity->getComponent<TransformComponent>()->getMatrixWithUpdate()[3];

	m_searchingClock += dt;
	// Change condition to not use a clock to get new directions
	if (m_searchingClock > 5.f) {
		int index = -1;
		if (aiComp->currPath.size() > 0) {
			index = aiComp->currPath[aiComp->currNodeIndex].index;
		}
		findRandomNodeIndex(index);
		// Save this comment to later track how to fine the way.
		//Logger::Log("node: " + std::to_string(m_targetNode) + " clock: " + std::to_string(clock));
		m_searchingClock = 0.f;
	}
	/*if (aiComp->entityTarget != nullptr) {
		auto enemyPos = aiComp->entityTarget->getComponent<TransformComponent>()->getMatrixWithUpdate()[3];
		m_distToHost = glm::distance2(enemyPos, aiPos);*/
		

		// This is can be rewritten in a better way
	aiComp->posTarget = m_nodeSystem->getNodes()[m_targetNode].position;
	aiComp->doWalk = true;

	if (m_targetNode != -1 && aiComp->timeTakenOnPath > 2.f) {
		aiComp->posTarget = m_nodeSystem->getNodes()[m_targetNode].position;
		aiComp->doWalk = true;
		aiComp->updatePath = true;
		aiComp->reachedPathingTarget = false;
	}
	//}
	

}

void SearchingState::reset(Entity* entity) {
	m_searchingClock = 0.f;
}

void SearchingState::init(Entity* entity) {
	m_searchingClock = 4.f;
}

float* SearchingState::getDistToHost() {
	return &m_distToHost;
}

void SearchingState::findRandomNodeIndex(int currNodeIndex) {
	//m_targetNode = m_nodeSystem->getNearestNode(pos + glm::vec3(float(std::rand() % 10) - 5.f, float(std::rand() % 10) - 5.f, float(std::rand() % 10) - 5.f)).index;
	bool foundIndex = false;
	while (!foundIndex) {
		int randX = (std::rand() % 10) - 5;
		int randZ = (std::rand() % 10) - 5;
		m_targetNode = currNodeIndex;
		m_targetNode += randX + randZ * m_nodeSystem->getXMax();
		m_targetNode %= (m_nodeSystem->getXMax() * m_nodeSystem->getZMax());
		m_targetNode = std::max(m_targetNode, 0);
		if (!m_nodeSystem->getNodes()[m_targetNode].blocked) {
			foundIndex = true;
		}
	}
}
