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

void NodeSystem::setNodes(const std::vector<Node>& nodes, const std::vector<std::vector<unsigned int>>& connections, unsigned int xMax, unsigned int zMax) {
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
		m_nodeEntities[currNodeEntity]->addComponent<TransformComponent>(m_nodes[i].position)->setScale(0.5f);
		m_nodeEntities[currNodeEntity]->addComponent<RealTimeComponent>();
		m_nodeEntities[currNodeEntity]->addComponent<CullingComponent>();
		m_nodeEntities[currNodeEntity]->addComponent<RenderInActiveGameComponent>();
		if (m_nodes[i].blocked) {
			auto blockedNodeModel = &Application::getInstance()->getResourceManager().getModelCopy("sphere.fbx", m_shader);
			blockedNodeModel->getMesh(0)->getMaterial()->setAlbedoTexture("missing.tga");
			blockedNodeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
			blockedNodeModel->setCastShadows(false);
			m_nodeEntities[currNodeEntity++]->addComponent<ModelComponent>(blockedNodeModel);
		} else {
			auto nodeModel = &Application::getInstance()->getResourceManager().getModelCopy("sphere.fbx", m_shader);
			nodeModel->getMesh(0)->getMaterial()->setAlbedoTexture("missing.tga");
			nodeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.f, 1.f, 0.f, 1.f));
			nodeModel->setCastShadows(false);
			m_nodeEntities[currNodeEntity++]->addComponent<ModelComponent>(nodeModel);
		}
	}


	for (int i = 0; i < m_nodes.size(); i++) {
		auto currNodeConnections = m_connections[i];
		for (int j = 0; j < currNodeConnections.size(); j++) {
			glm::vec3 pos = m_nodes[i].position;
			glm::vec3 dir = m_nodes[currNodeConnections[j]].position - pos;
			pos += glm::normalize(dir) * glm::length(dir) * 0.5f;
			m_nodeEntities.push_back(ECS::Instance()->createEntity("Connection " + std::to_string(i)));
			m_nodeEntities[currNodeEntity]->addComponent<TransformComponent>(pos)->setScale(0.25f);
			m_nodeEntities[currNodeEntity]->addComponent<RealTimeComponent>();
			m_nodeEntities[currNodeEntity]->addComponent<ModelComponent>(m_connectionModel);
			m_nodeEntities[currNodeEntity]->addComponent<CullingComponent>();
			m_nodeEntities[currNodeEntity++]->addComponent<RenderInActiveGameComponent>();
		}
	}
#endif
}

std::vector<NodeSystem::Node> NodeSystem::getPath(const NodeSystem::Node& from, const NodeSystem::Node& to) {
	std::vector<NodeSystem::Node> nPath;
	if (from.index != to.index && !m_nodes[to.index].blocked && !m_nodes[from.index].blocked && m_connections[to.index].size() > 0 && m_connections[from.index].size() > 0) {
		auto start = std::chrono::high_resolution_clock::now();
		auto path = aStar(from.index, to.index);
		m_pathSearchTimes[m_currSearchTimeIndex % NUM_SEARCH_TIMES] = 
			static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count());

		/*if (m_pathSearchTimes[m_currSearchTimeIndex % NUM_SEARCH_TIMES] > 5000) {
			SAIL_LOG("Finding a path (" + Utils::toStr(from.position) + "->" + Utils::toStr(to.position) + ") took " + 
					 std::to_string(static_cast<float>(m_pathSearchTimes[m_currSearchTimeIndex % NUM_SEARCH_TIMES]) / 1000.f) + "ms, size of path: " + std::to_string(path.size()));
		}*/
		m_currSearchTimeIndex++;
		//auto path = BFS(from.index, to.index);
		
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
		if (!m_nodes[i].blocked) {
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

const unsigned int NodeSystem::getAverageSearchTime() const {
	float searchTime = 0.f;
	for (unsigned int i = 0; i < std::min(m_currSearchTimeIndex, NUM_SEARCH_TIMES); i++) {
		searchTime += m_pathSearchTimes[i];
	}
	return searchTime / static_cast<float>(std::min(m_currSearchTimeIndex, NUM_SEARCH_TIMES));
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
void NodeSystem::setDebugModelAndScene(Shader* shader) {
	m_shader = shader;

	m_connectionModel = &Application::getInstance()->getResourceManager().getModelCopy("sphere.fbx", m_shader);
	m_connectionModel->getMesh(0)->getMaterial()->setAlbedoTexture("missing.tga");
	m_connectionModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.f, 1.f, 1.f, 1.f));
	m_connectionModel->setCastShadows(false);
}

std::vector<Entity::SPtr>& NodeSystem::getNodeEntities() {
	return m_nodeEntities;
}
#endif

#ifdef DEVELOPMENT
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