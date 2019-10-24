#include "pch.h"
#include "State.h"
#include "StateStack.h"

#include "imgui.h"

State::State(StateStack& stack) 
:  m_stack(&stack)
{

}

State::~State() {
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

void State::requestStackPush(States::ID stateID) {
	m_stack->pushState(stateID);
}
void State::requestStackPop() {
	m_stack->popState();
}
void State::requestStackClear() {
	m_stack->clearStack();
}
