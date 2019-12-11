#pragma once

#include "Component.h"
#include "../Entity.h"
#include "Sail/ai/states/FiniteStateMachine.h"
#include "Sail/entities/ECS.h"

class FSMComponent : public Component<FSMComponent>, public FiniteStateMachine {
public:
	FSMComponent()
		: FiniteStateMachine("FSM Component")
	{}

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		return sizeof(*this);
	}

	void imguiRender(Entity** selected) {
		ImGui::Text(("Current state: " + m_currentState->getName()).c_str());
	}
#endif
};