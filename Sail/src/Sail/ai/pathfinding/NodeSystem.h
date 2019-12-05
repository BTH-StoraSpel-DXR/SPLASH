#pragma once

#include <map>
#include <list>

//Keep this define in case debugging is needed
//#define _DEBUG_NODESYSTEM

#ifdef _DEBUG_NODESYSTEM
#include "Sail/entities/Entity.h"
class Shader;
#endif

class Model;

/*
	Checks if container of type U<X> contains element of type X.

	Examples of usage:
	Can be used to see if std::list<int> contains a specific int.
	Can also be used to see if std::vector<char> contains a specific char.
	etc.
	etc.
*/
template<template<class T> class U, class X> bool contains(U<X>& container, X& element) {
	for (X& x : container) {
		if (x == element) {
			return true;
		}
	}
	return false;
};

class NodeSystem {
public:
	struct Node {
		glm::vec3 position;
		bool blocked;
		unsigned int index;
		Node(glm::vec3 pos, bool blocked, unsigned int i)
			: position(pos)
			, blocked(blocked)
			, index(i)
		{}
	};

	NodeSystem();
	~NodeSystem();

	void setNodes(const std::vector<Node>& nodes, const std::vector<std::vector<unsigned int>>& connections, unsigned int xMax, unsigned int zMax);

	std::vector<NodeSystem::Node> getPath(const NodeSystem::Node& from, const NodeSystem::Node& to);
	std::vector<NodeSystem::Node> getPath(const glm::vec3& from, const glm::vec3& to);

	const NodeSystem::Node& getNearestNode(const glm::vec3& position) const;
	unsigned int getDistance2(unsigned int n1, unsigned int n2) const;
	const std::vector<NodeSystem::Node>& getNodes() const;
	const unsigned int getAverageSearchTime() const;
	const unsigned int getXMax() const;
	const unsigned int getZMax() const;

	void stop();

#ifdef _DEBUG_NODESYSTEM
	void setDebugModelAndScene(Shader* shader);
	std::vector<Entity::SPtr>& getNodeEntities();
	Model* m_connectionModel;
	std::vector<Entity::SPtr> m_nodeEntities;
	Shader* m_shader;
#endif
#ifdef DEVELOPMENT
	unsigned int getByteSize() const;
#endif

private:
	std::vector<unsigned int> BFS(const unsigned int from, const unsigned int to);
	std::vector<unsigned int> aStar(const unsigned int from, const unsigned int to);
	std::vector<std::vector<unsigned int>> m_connections;
	std::vector<NodeSystem::Node> m_nodes;

	const static unsigned int NUM_SEARCH_TIMES = 10;
	float m_pathSearchTimes[NUM_SEARCH_TIMES];
	unsigned int m_currSearchTimeIndex = 0;

	unsigned int m_xMax = 0;
	unsigned int m_zMax = 0;
};