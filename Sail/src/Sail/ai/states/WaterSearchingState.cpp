#include "pch.h"
#include "WaterSearchingState.h"

#include "Sail/ai/pathfinding/NodeSystem.h"
#include "Sail.h"

WaterSearchingState::WaterSearchingState(NodeSystem* nodeSystem)
	: m_nodeSystemRef(nodeSystem)
	, m_rendererWrapperRef(Application::getInstance()->getRenderWrapper())
	, m_targetPos(0.f, 0.f, 0.f)
	, m_searchingClock(4.f)
	, m_doSwitch(false) {
	m_stateTimer = 0.f;
}

WaterSearchingState::~WaterSearchingState() {}

void WaterSearchingState::update(float dt, Entity* entity) {
	m_stateTimer += dt;

	auto aiComp = entity->getComponent<AiComponent>();
	auto aiPos = entity->getComponent<TransformComponent>()->getMatrixWithUpdate()[3];

	m_searchingClock += dt;
	if ((aiComp->currNodeIndex >= aiComp->currPath.size() - 1) || aiComp->currPath.size() == 0) {
		int index = -1;
		if (aiComp->currPath.size() > 0) {
			index = aiComp->currPath[aiComp->currNodeIndex].index;
		} else {
			index = m_nodeSystemRef->getNearestNode(aiPos).index;
		}

		findRandomNodeIndex(-1);
		aiComp->updatePath = true;
		aiComp->posTarget = m_targetPos;
	} if ((m_searchingClock > 2.f && aiComp->currPath.size() / 2 <= aiComp->currNodeIndex || m_searchingClock > 5.f) && entity->getChildEntities().size() == 0) {
		int index = -1;
		if (aiComp->currPath.size() > 0) {
			index = aiComp->currPath[aiComp->currNodeIndex].index;
		} else {
			index = m_nodeSystemRef->getNearestNode(aiPos).index;
		}

		m_searchingClock = glm::linearRand(-2.f, 1.f);

		if (searchForWater(m_nodeSystemRef->getNodes()[index].position, index)) {
			m_doSwitch = true;
		}
		aiComp->posTarget = m_targetPos;
		aiComp->updatePath = true;
	}

	aiComp->doWalk = true;
}

void WaterSearchingState::reset(Entity* entity) {
	m_doSwitch = false;
	auto aiComp = entity->getComponent<AiComponent>();
	if (aiComp) {
		aiComp->automaticallyUpdatePath = true;
	}
}

void WaterSearchingState::init(Entity* entity) {
	m_stateTimer = 0.f;
	m_targetPos = glm::vec3(0.f, 0.f, 0.f);
	m_searchingClock = 0.f;
	auto aiComp = entity->getComponent<AiComponent>();
	if (aiComp) {
		aiComp->automaticallyUpdatePath = false;
	}
	m_doSwitch = false;

	findRandomNodeIndex(-1);
	aiComp->updatePath = true;
	aiComp->posTarget = m_targetPos;
	aiComp->currNodeIndex = 0;
}

bool* WaterSearchingState::getDoSwitch() {
	return &m_doSwitch;
}

void WaterSearchingState::findRandomNodeIndex(int currNodeIndex, int offsetX, int offsetZ) {
	int targetNode = 0;
	if (currNodeIndex == -1) {
		targetNode = std::rand() % m_nodeSystemRef->getNodes().size();
	} else {
		bool foundIndex = false;
		int iterations = 0;
		while (!foundIndex) {
			int randX = (std::rand() % offsetX * 2) - offsetX;
			int randZ = (std::rand() % offsetZ * 2) - offsetZ;
			targetNode = currNodeIndex;
			targetNode += randX + randZ * m_nodeSystemRef->getXMax();
			targetNode %= (m_nodeSystemRef->getXMax() * m_nodeSystemRef->getZMax());
			targetNode = std::max(targetNode, 0);
			if (!m_nodeSystemRef->getNodes()[targetNode].blocked) {
				foundIndex = true;
			}
			iterations++;

			if (iterations > 10) {
				foundIndex = true;
				targetNode = std::rand() % m_nodeSystemRef->getNodes().size();
			}
		}
	}

	m_targetPos = m_nodeSystemRef->getNodes()[targetNode].position;
}

bool WaterSearchingState::searchForWater(const glm::vec3& currPos, const int currNodeIndex) {
	glm::vec3 maxOffset(30.f, 0.f, 30.f);
	auto found = m_rendererWrapperRef->getNearestWaterPosition(currPos, maxOffset);
	if (found.first) {
		m_targetPos = found.second;
	}
	return found.first;
}
