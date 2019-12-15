#include "pch.h"
#include "NodeSystem.h"
#include "Sail.h"

#ifdef _DEBUG_NODESYSTEM
#include "Sail/entities/ECS.h"
#endif

NodeSystem::NodeSystem() {

}

NodeSystem::~NodeSystem() {

}

void NodeSystem::setNodes(const std::vector<Node>& nodes, const std::vector<std::vector<unsigned int>>& connections, const unsigned int xMax, const unsigned int zMax) {
#ifdef _DEBUG_NODESYSTEM
	if ( m_shader == nullptr ) {
		SAIL_LOG_ERROR("Shader need to be set in the node system during debug.");
	}
#endif

	m_xMax = xMax;
	m_zMax = zMax;

	m_nodes = nodes;
	m_connections = connections;

#ifdef _DEBUG_NODESYSTEM
	int currNodeEntity = 0;
	for ( int i = 0; i < m_nodes.size(); i++ ) {
		m_nodeEntities.push_back(ECS::Instance()->createEntity("Node " + std::to_string(i)));
		m_nodeEntities[currNodeEntity]->addComponent<TransformComponent>(m_nodes[i].position)->setScale(0.2f);
		m_nodeEntities[currNodeEntity]->addComponent<RealTimeComponent>();
		m_nodeEntities[currNodeEntity]->addComponent<CullingComponent>();
		m_nodeEntities[currNodeEntity]->addComponent<RenderInActiveGameComponent>();
		if (m_nodes[i].blocked) {
			m_nodeEntities[currNodeEntity++]->addComponent<ModelComponent>(m_blockedNode);
		} else {
			m_nodeEntities[currNodeEntity++]->addComponent<ModelComponent>(m_pathNodes[m_maxColourID]);
		}
	}


	for (int i = 0; i < m_nodes.size(); i++) {
		auto currNodeConnections = m_connections[i];
		m_connectionEntities.push_back(std::vector<std::pair<unsigned int, Entity::SPtr>>());
		int currConnIndex = 0;
		for (int j = 0; j < currNodeConnections.size(); j++) {
			// Only add forward-connections (to save mesh space)
			if (currNodeConnections[j] > i) {
				glm::vec3 pos = m_nodes[i].position;
				glm::vec3 dir = m_nodes[currNodeConnections[j]].position - pos;
				auto dirLength = glm::length(dir);
				auto normalizedDir = glm::normalize(dir);
				pos += normalizedDir * dirLength * 0.5f;
				m_connectionEntities[i].emplace_back(currNodeConnections[j], ECS::Instance()->createEntity("Connection " + std::to_string(i)).get());
				float dirX = normalizedDir.x;
				float dirZ = normalizedDir.z;
				// To avoid division by zero
				dirZ = dirZ != 0 ? dirZ : 0.001f;
				float yaw = 0.f;
				if (dirZ < 0.f) {
					yaw = glm::atan(dirX / dirZ) - glm::pi<float>();
				} else {
					yaw = glm::atan(dirX / dirZ);
				}

				auto transComp = m_connectionEntities[i][currConnIndex].second->addComponent<TransformComponent>(pos);
				transComp->setRotations(0.f, yaw, 0.f);
				transComp->setScale(0.01f, 0.01f, dirLength * 1.5f);
				m_connectionEntities[i][currConnIndex].second->addComponent<RealTimeComponent>();

				m_connectionEntities[i][currConnIndex].second->addComponent<ModelComponent>(m_pathNodes[m_maxColourID]);
				m_connectionEntities[i][currConnIndex].second->addComponent<CullingComponent>();
				m_connectionEntities[i][currConnIndex++].second->addComponent<RenderInActiveGameComponent>();
			}
		}
	}
#endif
}

std::vector<NodeSystem::Node> NodeSystem::getPath(const NodeSystem::Node& from, const NodeSystem::Node& to) {
	std::vector<NodeSystem::Node> nPath;
	if (from.index != to.index && !m_nodes[to.index].blocked && !m_nodes[from.index].blocked && m_connections[to.index].size() > 0 && m_connections[from.index].size() > 0) {
#ifdef DEVELOPMENT
		auto start = std::chrono::high_resolution_clock::now();
#endif
		auto path = aStar(from.index, to.index);
#ifdef DEVELOPMENT
		m_pathSearchTimes[m_currSearchTimeIndex % NUM_SEARCH_TIMES] = 
			static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count());
		if (m_pathSearchTimes[m_currSearchTimeIndex % NUM_SEARCH_TIMES] > 5000) {
			SAIL_LOG("Finding a path (" + Utils::toStr(from.position) + "->" + Utils::toStr(to.position) + ") took " + 
					 std::to_string(static_cast<float>(m_pathSearchTimes[m_currSearchTimeIndex % NUM_SEARCH_TIMES]) / 1000.f) + "ms, size of path: " + std::to_string(path.size()));
		}
		m_currSearchTimeIndex++;
#endif
		
		size_t size = path.size();
		for ( size_t i = size - 1; i < size; i-- ) {
			nPath.push_back(m_nodes[path[i]]);
		}
	}
	return nPath;
}

std::vector<NodeSystem::Node> NodeSystem::getPath(const glm::vec3& from, const glm::vec3& to) {
	return getPath(getNearestNode(from), getNearestNode(to));
}

const NodeSystem::Node& NodeSystem::getNearestNode(const glm::vec3& position) const {
	float dist = FLT_MAX;
	unsigned int index = 0;

	for ( unsigned int i = 0; i < m_nodes.size(); i++ ) {
		if (!m_nodes[i].blocked && m_connections[m_nodes[i].index].size() > 0) {
			float d = glm::distance2(m_nodes[i].position, position);
			if (d < dist && !m_nodes[i].blocked) {
				index = i;
				dist = d;
			}
		}
	}

	return m_nodes[index];
}

unsigned int NodeSystem::getDistance2(unsigned int n1, unsigned int n2) const {
	return glm::distance2(m_nodes[n1].position, m_nodes[n2].position); // TOOD: Check this - should be ceil, floor or round
}

const std::vector<NodeSystem::Node>& NodeSystem::getNodes() const {
	return m_nodes;
}

const std::vector<std::vector<unsigned int>>& NodeSystem::getConnections() const {
	return m_connections;
}

const unsigned int NodeSystem::getXMax() const {
	return m_xMax;
}

const unsigned int NodeSystem::getZMax() const {
	return m_zMax;
}

void NodeSystem::stop() {
#ifdef _DEBUG_NODESYSTEM
	for (int i = 0; i < m_nodeEntities.size(); i++) {
		ECS::Instance()->queueDestructionOfEntity(m_nodeEntities[i].get());
	}
	m_nodeEntities.clear();
#endif

	m_nodes.clear();
	m_connections.clear();
}

#ifdef _DEBUG_NODESYSTEM
void NodeSystem::colorPath(const std::vector<NodeSystem::Node>& path, const unsigned int colourID) {
	Model* modelToSet = m_pathNodes[colourID];
	for (int i = 0; i < path.size(); i++) {
		// Colour all affected nodes
		m_nodeEntities[path[i].index]->getComponent<ModelComponent>()->setModel(modelToSet);

		// Colour all affected backward-connections
		if (i > 0) {
			for (int j = 0; j < m_connectionEntities[path[i].index].size(); j++) {
				if (m_connectionEntities[path[i].index][j].first == path[i - 1].index) {
					m_connectionEntities[path[i].index][j].second->getComponent<ModelComponent>()->setModel(modelToSet);
				}
			}
		}
		// Colour all affected forward-connections
		if (i < path.size() - 1) {
			for (int j = 0; j < m_connectionEntities[path[i].index].size(); j++) {
				if (m_connectionEntities[path[i].index][j].first == path[i + 1].index) {
					m_connectionEntities[path[i].index][j].second->getComponent<ModelComponent>()->setModel(modelToSet);
				}
			}
		}
	}
}

void NodeSystem::setDebugModelAndScene(Shader* shader) {
	m_shader = shader;

	Application::getInstance()->getResourceManager().getModel("NodeSystemBall", m_shader);

	m_blockedNode = &Application::getInstance()->getResourceManager().getModelCopy("NodeSystemBall", m_shader);
	m_blockedNode->getMesh(0)->getMaterial()->setAlbedoTexture("missing.tga");
	m_blockedNode->getMesh(0)->getMaterial()->setColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
	m_blockedNode->setCastShadows(false);

	for (int i = 0; i < m_maxColourID; i++) {
		m_pathNodes.emplace_back(&Application::getInstance()->getResourceManager().getModelCopy("NodeSystemBall", m_shader));
		m_pathNodes[i]->getMesh(0)->getMaterial()->setAlbedoTexture("missing.tga");
		m_pathNodes[i]->getMesh(0)->getMaterial()->setColor(glm::vec4(glm::linearRand(0.5f, 1.f), glm::linearRand(0.5f, 1.f), glm::linearRand(0.5f, 1.f), 1.f));
		m_pathNodes[i]->setCastShadows(false);
	}

	m_pathNodes.emplace_back(&Application::getInstance()->getResourceManager().getModelCopy("NodeSystemBall", m_shader));
	m_pathNodes[m_maxColourID]->getMesh(0)->getMaterial()->setAlbedoTexture("missing.tga");
	m_pathNodes[m_maxColourID]->getMesh(0)->getMaterial()->setColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.f));
	m_pathNodes[m_maxColourID]->setCastShadows(false);
}
#endif

#ifdef DEVELOPMENT
const unsigned int NodeSystem::getAverageSearchTime() const {
	float searchTime = 0.f;
	for (unsigned int i = 0; i < std::min(m_currSearchTimeIndex, NUM_SEARCH_TIMES); i++) {
		searchTime += m_pathSearchTimes[i];
	}
	return searchTime / static_cast<float>(std::min(m_currSearchTimeIndex, NUM_SEARCH_TIMES));
}

unsigned int NodeSystem::getByteSize() const {
	unsigned int size = sizeof(*this);
	size += m_connections.size() * sizeof(std::vector<unsigned int>);
	for (auto& vec : m_connections) {
		size += vec.size() * sizeof(unsigned int);
	}
	size += m_nodes.size() * sizeof(NodeSystem::Node);
	return size;
}
#endif

std::vector<unsigned int> NodeSystem::BFS(const unsigned int from, const unsigned int to) {
	std::list<unsigned int> queue;
	std::vector<unsigned int> visited = std::vector<unsigned int>(m_connections.size());
	std::vector<unsigned int> path;
	std::vector<float> dist(m_connections.size(), FLT_MAX);
	std::vector<unsigned int> traceBack(m_connections.size(), INT_MAX);

	queue.emplace_back(from);
	visited[from] = true;
	dist[from] = 0;
	while ( !queue.empty() ) {
		unsigned int currNode = queue.front();
		queue.pop_front();

		for ( auto node : m_connections[currNode] ) {
			if ( !visited[node] ) {
				queue.push_back(node);
				visited[node] = true;
				traceBack[node] = currNode;
				dist[node] = dist[currNode] + 1;
			}
		}
	}

	unsigned int currNode = to;
	while ( currNode != from ) {
		if ( dist[currNode] != FLT_MAX ) {
			path.emplace_back(currNode);
			currNode = traceBack[currNode];
		}
		else {
			return std::vector<unsigned int>(0);
		}
	}
	path.emplace_back(from);

	return path;
}

/*
Can be optimized
*/
std::vector<unsigned int> NodeSystem::aStar(const unsigned int from, const unsigned int to) {
	std::list<unsigned int> openSet;
	std::list<unsigned int> closedSet;
	std::vector<unsigned int> path;

	openSet.push_back(from);
	unsigned int current = from;
	unsigned int maxNodes = (unsigned int)m_nodes.size();
	unsigned int* camefrom = SAIL_NEW unsigned int[maxNodes];
	unsigned int* gScores = SAIL_NEW unsigned int[maxNodes];
	unsigned int* fScores = SAIL_NEW unsigned int[maxNodes];

	memset(gScores, UCHAR_MAX, maxNodes * sizeof(unsigned int));
	memset(fScores, UCHAR_MAX, maxNodes * sizeof(unsigned int));

	gScores[from] = 0;
	fScores[from] = 0;
	camefrom[from] = from;

	unsigned int numNodesChecked = 0;
	while (!openSet.empty() && numNodesChecked < 100) {
		current = openSet.front();
		for (unsigned int n : openSet) {
			if (fScores[n] < fScores[current]) {
				current = n;
			}
		}

		if (current == to) {
			path.push_back(to);

			int cur = to;
			while (camefrom[cur] != cur) {
				path.push_back(camefrom[cur]);
				cur = camefrom[cur];
			}

			break;
		} else {
			openSet.remove((unsigned int)(current));
			closedSet.push_back(current);
			
			for (auto neighbor : m_connections[current]) {
				if (m_nodes[neighbor].blocked || contains<std::list, unsigned int>(closedSet, neighbor)) {
					continue;
				} else {
					//Distance From Start To Neighbor Through Current
					unsigned int dist = gScores[current] + getDistance2(current, neighbor);
					if (dist < gScores[neighbor]) {
						camefrom[neighbor] = current;
						gScores[neighbor] = dist;
						fScores[neighbor] = dist + getDistance2(neighbor, to);
						
						if (!contains<std::list, unsigned int>(openSet, neighbor)) {
							openSet.push_back(neighbor);
						}
					}
				}
			}
		}
		numNodesChecked++;
	}

	delete[] camefrom;
	delete[] gScores;
	delete[] fScores;

 	return path;
}