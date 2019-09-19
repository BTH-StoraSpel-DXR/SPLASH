#pragma once

#include "pch.h"

template<class T>
class Node {
public:
	Node(T* data = nullptr, Node<T>* parent = nullptr) : m_data(data), m_parent(parent) {
		if (m_parent)
			m_parent->addChild(this);
	};
	virtual ~Node() {};


	void setParent(Node<T>* parent) {
		if (m_parent) {
			m_parent->removeChild(this);
		}
		m_parent = parent;
		parent->addChild(this);
		m_parentUpdated = true;
		treeNeedsUpdating();
	}

	void removeParent() {
		if (m_parent) {
			m_parent->removeChild(this);
			m_parent = nullptr;
		}
	}

	void addChild(Node<T>* child) {
		m_children.push_back(child);
	}

	void removeChild(Node<T>* child) {
		for (int i = 0; i < m_children.size(); i++) {
			if (m_children[i] == child) {
				m_children[i] = m_children.back();
				m_children.pop_back();
				break;
			}
		}
	}

	void treeNeedsUpdating() {
		m_parentUpdated = true;
		for (Node<T>* child : m_children) {
			child->treeNeedsUpdating();
		}
	}

	bool hasParent() const {
		return m_parent;
	}

	bool getParentUpdated() const {
		return m_parentUpdated;
	}

	void setParentUpdated(bool updated) {
		m_parentUpdated = updated;
	}

	T* getDataPtr() {
		return m_data;
	}

	Node<T>* getParent() {
		return m_parent;
	}
protected:
	T* m_data = nullptr;

	Node<T>* m_parent = nullptr;
	std::vector<Node<T>*> m_children;
private:

	bool m_parentUpdated = false;
};