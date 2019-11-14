#include "InGameMenuState.h"
#include "imgui.h"
#include "Sail/api/Input.h"
#include "Sail/Application.h"
#include "Network/NWrapperSingleton.h"
#include "../events/GameOverEvent.h"
#include "Sail/events/EventDispatcher.h"

bool InGameMenuState::sIsOpen = false;

InGameMenuState::InGameMenuState(StateStack& stack) 
	: State(stack) 
{
	sIsOpen = true;
	m_isSinglePlayer = NWrapperSingleton::getInstance().getPlayers().size() == 1;
}

InGameMenuState::~InGameMenuState(){
	sIsOpen = false;
}

bool InGameMenuState::processInput(float dt) {
#ifndef DEVELOPMENT
	// Show mouse
	Input::HideCursor(false);
#endif
	if (m_inGameMenuWindow.popGameState()) {
		// Reset the network
		NWrapperSingleton::getInstance().resetNetwork();
		NWrapperSingleton::getInstance().resetWrapper();
		// Dispatch game over event
		EventDispatcher::Instance().emit(GameOverEvent());

		this->requestStackPop();
		this->requestStackPop();
		this->requestStackPush(States::MainMenu);
	} else if (m_inGameMenuWindow.exitInGameMenu()) {
		this->requestStackPop();
	}

	return false;
}

bool InGameMenuState::update(float dt, float alpha) {
	return !m_isSinglePlayer;
}

bool InGameMenuState::fixedUpdate(float dt) {
	return !m_isSinglePlayer;
}

bool InGameMenuState::render(float dt, float alpha) {
	return true;
}

bool InGameMenuState::renderImgui(float dt) {
	m_inGameMenuWindow.renderWindow();
	if (m_inGameMenuWindow.showOptions()) {
		



	}

	return false;
}

bool InGameMenuState::IsOpen() {
	return sIsOpen;
}
