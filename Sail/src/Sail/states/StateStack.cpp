#include "pch.h"
#include "StateStack.h"
#include "Sail/Application.h"
#include "Sail/api/Input.h"
#include "Sail/KeyCodes.h"

StateStack::StateStack()
	: m_renderImgui(true)
{
}

State::Ptr StateStack::createState(States::ID stateID) {

	// Get the initializer function for the specified id
	auto found = m_factories.find(stateID);
	// A function should always exist, assert otherwise
	assert(found != m_factories.end());

	// Return the pointer to the newly initiated state
	return found->second();

}

void StateStack::processInput(float dt) {

	// Toggle imgui rendering on key
	if (Input::WasKeyJustPressed(SAIL_KEY_F10))
		m_renderImgui = !m_renderImgui;

	// Loop through the stack reversed
	for (auto itr = m_stack.rbegin(); itr != m_stack.rend(); ++itr) {

		// Return if a state returns false
		// This allows states to stop underlying states from handling events
		if (!(*itr)->processInput(dt))
			break;

	}

	// Loop done, we can now modify the stack
	applyPendingChanges();

}

void StateStack::update(float dt) {

	// Loop through the stack reversed
	for (auto itr = m_stack.rbegin(); itr != m_stack.rend(); ++itr) {

		// Return if a state returns false
		// This allows states to stop underlying states from updating
		if (!(*itr)->update(dt))
			break;

	}
	applyPendingChanges();

}

void StateStack::render(float dt) {

	// Loop through the states and draw them all
	/*for (int i = m_stack.size() - 1; i >= 0; i--) {
		m_stack.at(i)->render(dt);
	}*/
	for (auto& state : m_stack)
		state->render(dt);
	
	if (m_renderImgui) {
		Application::getInstance()->getImGuiHandler()->begin();
		for (auto& state : m_stack)
			state->renderImgui(dt);
		Application::getInstance()->getImGuiHandler()->end();
	}

	Application::getInstance()->getAPI()->present(false);
}

void StateStack::onEvent(Event& event) {
	// Loop through the stack reversed
	for (auto itr = m_stack.rbegin(); itr != m_stack.rend(); ++itr) {

		// Return if a state returns false
		// This allows states to stop underlying states from receiving an event
		if (!((*itr)->onEvent(event)))
			break;

	}
}

void StateStack::pushState(States::ID stateID) {
	m_pendingList.push_back(PendingChange(Push, stateID));
}
void StateStack::popState() {
	m_pendingList.push_back(PendingChange(Pop));
}
void StateStack::clearStack() {
	m_pendingList.push_back(PendingChange(Clear));
}
bool StateStack::isEmpty() const {
	return m_stack.size() == 0;
}

void StateStack::applyPendingChanges() {
	
	// Perform something depending on the action
	for (auto change : m_pendingList) {
		switch (change.action) {
		case Push:
			m_stack.push_back(createState(change.stateID));
			break;

		case Pop:
			m_stack.pop_back();
			break;

		case Clear:
			m_stack.clear();
			break;
		}
	}

	// All changes applied, clear the list
	m_pendingList.clear();

}

StateStack::PendingChange::PendingChange(Action action, States::ID stateID)
: action(action)
, stateID(stateID)
{}