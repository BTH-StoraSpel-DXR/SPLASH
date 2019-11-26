#include "pch.h"
#include "InGameMenuWindow.h"
#include "Sail/Application.h"
#include "Sail/KeyBinds.h"

InGameMenuWindow::InGameMenuWindow(bool showWindow)
	: m_popGameState(false)
	, m_exitInGameMenu(false)
	, m_showOptions(false)
{
	m_openedThisFrame = true;
	Input::HideCursor(false);




}

InGameMenuWindow::~InGameMenuWindow() {}

void InGameMenuWindow::renderWindow() {
	// Rendering a pause window in the middle of the game window.
	if (!m_openedThisFrame && ImGui::IsKeyPressed(KeyBinds::SHOW_IN_GAME_MENU, false)) {
		m_exitInGameMenu = true;
		Input::HideCursor(true);
	}
	m_openedThisFrame = false;
}

bool InGameMenuWindow::popGameState() {
	return m_popGameState;
}

bool InGameMenuWindow::exitInGameMenu() {
	return m_exitInGameMenu;
}

bool InGameMenuWindow::showOptions() {
	return m_showOptions;
}
