#pragma once

#include "Component.h"
#include "../Entity.h"
#include "Sail/ai/pathfinding/NodeSystem.h"
#include <glm/glm.hpp>

class AiComponent : public Component<AiComponent> {
public:
	AiComponent() 
		: timeTakenOnPath(0.f)
		, timeBetweenPathUpdate(3.f)
		, updatePath(true)
		, doWalk(false)
		, automaticallyUpdatePath(true)
		, currNodeIndex(0)
		, controlledEntity(nullptr)
		, posTarget(glm::vec3(0.f, 0.f, 0.f))
	{}

	float timeTakenOnPath;
	float timeBetweenPathUpdate;
	
	bool updatePath;
	bool doWalk;
	bool automaticallyUpdatePath;

	int currNodeIndex;

	std::vector<NodeSystem::Node> currPath;

	Entity* controlledEntity;

	glm::vec3 posTarget;

public:
#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this) + sizeof(NodeSystem::Node) * currPath.size();
	}

	void imguiRender(Entity** selected) {
		ImGui::Text(("timeTakenOnPath " + std::to_string(timeTakenOnPath)).c_str());
		ImGui::Text(("timeBetweenPathUpdate " + std::to_string(timeBetweenPathUpdate)).c_str());

		ImGui::Text(("updatePath " + std::to_string(updatePath)).c_str());
		ImGui::Text(("doWalk " + std::to_string(doWalk)).c_str());
		ImGui::Text(("automaticallyUpdatePath " + std::to_string(automaticallyUpdatePath)).c_str());

		ImGui::Text(("currNodeIndex " + std::to_string(currNodeIndex)).c_str());

		ImGui::Text(("posTarget " + Utils::toStr(posTarget)).c_str());
	}
#endif
};