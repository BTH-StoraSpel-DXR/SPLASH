#include "pch.h"
#include "CleaningState.h"

#include "Sail/ai/pathfinding/NodeSystem.h"
#include "Sail.h"

CleaningState::CleaningState(NodeSystem* nodeSystem) 
	: m_nodeSystemRef(nodeSystem)
	, m_rendererWrapperRef(Application::getInstance()->getRenderWrapper())
	, m_targetPos(0.f, 0.f, 0.f)
	, m_searchingClock(4.f)
	, m_doSwitch(false)
	, m_maxStateTime(10.f)
	, m_cleaningPathStart(0) {
	m_stateTimer = 0.f;
}

CleaningState::~CleaningState() {}

void CleaningState::update(float dt, Entity* entity) {
	m_stateTimer += dt;

	auto aiComp = entity->getComponent<AiComponent>();
	auto aiPos = entity->getComponent<TransformComponent>()->getMatrixWithUpdate()[3];
	
	aiComp->doWalk = true;

	if (m_stateTimer > m_maxStateTime || aiComp->currPath.size() == 0 || aiComp->currNodeIndex >= aiComp->currPath.size() - 1) {
		m_doSwitch = true;
	}

	if (aiComp->currNodeIndex >= m_cleaningPathStart) {
		auto speedLC = entity->getComponent<SpeedLimitComponent>();
		if (speedLC) {
			speedLC->maxSpeed = speedLC->normalMaxSpeed;
		}
	}
}

void CleaningState::reset(Entity* entity) {
	m_doSwitch = false;
	auto aiComp = entity->getComponent<AiComponent>();
	if (aiComp) {
		aiComp->automaticallyUpdatePath = true;
	}

	auto speedLC = entity->getComponent<SpeedLimitComponent>();
	if (speedLC) {
		speedLC->maxSpeed = speedLC->normalMaxSpeed;
	}
}

void CleaningState::init(Entity* entity) {
	m_stateTimer = 0.f;
	m_doSwitch = false;
	m_targetPos = glm::vec3(0.f, 0.f, 0.f);
	m_searchingClock = 4.f;
	auto aiComp = entity->getComponent<AiComponent>();
	if (aiComp) {
		aiComp->automaticallyUpdatePath = false;
	}
	m_maxStateTime = glm::linearRand(3.f, 10.f);
	// Create a path for the bot to walk
	createCleaningPath(entity);

	auto speedLC = entity->getComponent<SpeedLimitComponent>();
	if (speedLC) {
		speedLC->maxSpeed = glm::linearRand(1.2f, 2.f) * speedLC->normalMaxSpeed;
	}
}

bool* CleaningState::getDoSwitch() {
	return &m_doSwitch;
}

void CleaningState::createCleaningPath(Entity* entity) {// First try to walk to the left-bottom most index of the water
	glm::vec3 maxOffset(30.f, 0.f, 30.f);
	auto aiComp = entity->getComponent<AiComponent>();
	glm::vec3 aiPos = entity->getComponent<TransformComponent>()->getMatrixWithUpdate()[3];
	auto found = m_rendererWrapperRef->getNearestWaterPosition(aiPos, maxOffset);
	aiComp->posTarget = found.second;

	/* TODO: Should probably be improved so we don't need to copy this code */
	{
#ifdef _DEBUG_NODESYSTEM
		m_nodeSystemRef->colorPath(aiComp->currPath, m_nodeSystemRef->getMaxColourID());
#endif
		aiComp->timeTakenOnPath = 0.f;

		auto tempPath = m_nodeSystemRef->getPath(aiPos, aiComp->posTarget);

		aiComp->currNodeIndex = 0;

		// Fix problem of always going toward closest node
		if (tempPath.size() > 1 && glm::distance(tempPath[1].position, aiPos) < glm::distance(tempPath[1].position, tempPath[0].position)) {
			aiComp->currNodeIndex += 1;
		}

		aiComp->currPath = tempPath;

		aiComp->updatePath = false;
	}

	m_cleaningPathStart = aiComp->currPath.size();

	auto nodes = m_nodeSystemRef->getNodes();
	auto cleaningSize = glm::ivec2((std::rand() % 2) + 2, (std::rand() % 2) + 2);
	int x, y, dx, dy;
	x = y = dx = 0;
	dy = -1;
	int t = std::max(cleaningSize.x, cleaningSize.y);
	int maxI = t * t;
	for (int i = 0; i < maxI; i++) {
		if ((-cleaningSize.x / 2 <= x) && (x <= cleaningSize.x / 2) && (-cleaningSize.y / 2 <= y) && (y <= cleaningSize.y / 2)) {
			int currIndex;
			if (aiComp->currPath.size() > 0) {
				currIndex = aiComp->currPath[aiComp->currPath.size() - 1].index;
			} else {
				currIndex = m_nodeSystemRef->getNearestNode(aiPos).index;
			}

			std::vector<NodeSystem::Node> shortPath;
			int pathEndIndex = currIndex + x + y * m_nodeSystemRef->getXMax();
			if (currIndex > -1 && pathEndIndex > -1 && pathEndIndex < nodes.size()) {
				shortPath = m_nodeSystemRef->getPath(nodes[currIndex], nodes[pathEndIndex]);

				if (shortPath.size() > 0) {
					aiComp->currPath.insert(aiComp->currPath.end(), shortPath.begin() + 1, shortPath.end());
				}
			}
		}
		if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y))) {
			t = dx;
			dx = -dy;
			dy = t;
		}
		x += dx;
		y += dy;
	}

#ifdef _DEBUG_NODESYSTEM
	m_nodeSystemRef->colorPath(aiComp->currPath, entity->getID() % (m_nodeSystemRef->getMaxColourID() - 1));
#endif
}
