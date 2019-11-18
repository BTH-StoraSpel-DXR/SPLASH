#include "pch.h"
#include "State.h"
#include "StateStack.h"
#include "Sail/events/Events.h"

#include "imgui.h"

State::State(StateStack& stack) 
:  m_stack(&stack)
{
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_CHANGE_STATE, this);
}

State::~State() {
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_CHANGE_STATE, this);
}

bool State::fixedUpdate(float dt) {
	return false;
}

bool State::renderImgui(float dt) {
	ImGui::ShowDemoWindow();
	return false;
}

bool State::renderImguiDebug(float dt) {
	return false;
}

bool State::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::NETWORK_CHANGE_STATE: onHostStateChangeRequest((const NetworkChangeStateEvent&)event); break;
	default:
		break;
	}

	return true;
}

void State::onHostStateChangeRequest(const NetworkChangeStateEvent& event) {	
	if (event.stateID == States::JoinLobby) {
		requestStackPop();
	} else {
		requestStackClear();
	}
	requestStackPush(event.stateID);
}

void State::requestStackPush(States::ID stateID) {
	m_stack->pushState(stateID);
}
void State::requestStackPop() {
	m_stack->popState();
}
void State::requestStackClear() {
	m_stack->clearStack();
}
