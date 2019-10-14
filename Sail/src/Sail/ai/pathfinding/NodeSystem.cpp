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

void NodeSystem::setNodes(const std::vector<Node>& nodes, const std::vector<std::vector<unsigned int>>& connections) {
#ifdef _DEBUG_NODESYSTEM
	if ( m_nodeModel == nullptr ) {
		Logger::Error("Node model and scene need to be set in the node system during debug.");
	}
#endif

	m_nodes = nodes;
	m_connections = connections;

#ifdef _DEBUG_NODESYSTEM
	for ( int i = 0; i < m_nodeEntities.size(); i++ ) {
		ECS::Instance()->destroyEntity(m_nodeEntities[i]);
	}
	m_nodeEntities.clear();
	int currNodeEntity = 0;
	for ( int i = 0; i < m_nodes.size(); i++ ) {
		if ( !m_nodes[i].blocked ) {
			m_nodeEntities.push_back(ECS::Instance()->createEntity("Node " + std::to_string(i)));
			m_nodeEntities[currNodeEntity]->addComponent<TransformComponent>(m_nodes[i].position);
			m_nodeEntities[currNodeEntity++]->addComponent<ModelComponent>(m_nodeModel);
		}
	}
#endif
}

std::vector<NodeSystem::Node> NodeSystem::getPath(const NodeSystem::Node& from, const NodeSystem::Node& to) {
	std::vector<NodeSystem::Node> nPath;
	if ( from.index != to.index ) {
		auto path = aStar(from.index, to.index);
		
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
		float d = glm::distance(m_nodes[i].position, position);
		if ( d < dist && !m_nodes[i].blocked) {
			index = i;
			dist = d;
		}
	}

	return m_nodes[index];
}

unsigned int NodeSystem::getDistance(unsigned int n1, unsigned int n2) const {
	return glm::distance(m_nodes[n1].position, m_nodes[n2].position); // TOOD: Check this - should be ceil, floor or round
}

const std::vector<NodeSystem::Node>& NodeSystem::getNodes() const {
	return m_nodes;
}

#ifdef _DEBUG_NODESYSTEM
void NodeSystem::setDebugModelAndScene(Shader* shader) {
	m_nodeModel = &Application::getInstance()->getResourceManager().getModel("sphere.fbx", shader);
	m_nodeModel->getMesh(0)->getMaterial()->setDiffuseTexture("missing.tga");
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
			Logger::Warning("No path was found");
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

	while (!openSet.empty()) {
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
					unsigned int dist = gScores[current] + getDistance(current, neighbor);
					if (dist < gScores[neighbor]) {
						camefrom[neighbor] = current;
						gScores[neighbor] = dist;
						fScores[neighbor] = dist + getDistance(neighbor, to);
						
						if (!contains<std::list, unsigned int>(openSet, neighbor)) {
							openSet.push_back(neighbor);
						}
					}
				}
			}
		}
	}

	delete[] camefrom;
	delete[] gScores;
	delete[] fScores;

 	return path;
}