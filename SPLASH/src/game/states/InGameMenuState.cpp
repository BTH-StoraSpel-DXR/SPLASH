#include "InGameMenuState.h"
#include "imgui.h"
#include "Sail/api/Input.h"
#include "Sail/Application.h"
#include "Network/NWrapperSingleton.h"
#include "../events/GameOverEvent.h"
#include "Sail/events/EventDispatcher.h"
#include "Sail/utils/SailImGui/SailImGui.h"
bool InGameMenuState::sIsOpen = false;

InGameMenuState::InGameMenuState(StateStack& stack) 
	: State(stack) ,
	m_outerPadding(30)
{
	Input::HideCursor(false);
	m_app = Application::getInstance();
	m_imGuiHandler = m_app->getImGuiHandler();
	sIsOpen = true;
	m_isSinglePlayer = NWrapperSingleton::getInstance().getPlayers().size() == 1;
	m_openedThisFrame = true;
	
	m_standaloneButtonflags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBackground;
	m_backdrop = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus;
	m_backgroundOnlyflags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings;
	m_windowToRender = 0;


	m_outerPadding = 35;
	m_menuWidth = 700.0f;
	m_usePercentage = true;
	m_percentage = 0.35f;

	m_pos = ImVec2(0, 0);
	m_size = ImVec2(0, 0);
	m_minSize = ImVec2(435, 500);
	m_maxSize = ImVec2(1000, 1000);


}

InGameMenuState::~InGameMenuState(){
	sIsOpen = false;
}

bool InGameMenuState::processInput(float dt) {
#ifndef DEVELOPMENT
	// Show mouse
#endif
	if (m_inGameMenuWindow.popGameState()) {
		
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

	if (m_usePercentage) {
		m_menuWidth = m_percentage * m_app->getWindow()->getWindowWidth();
	}

	m_size.x = m_menuWidth;
	m_size.y = m_app->getWindow()->getWindowHeight() - m_outerPadding * 2;
	m_pos.x = m_app->getWindow()->getWindowWidth() - m_outerPadding - ((m_size.x < m_minSize.x) ? m_minSize.x : m_size.x);
	m_pos.y = m_outerPadding;



	renderMenu();
	if (m_windowToRender == 1) {
		renderOptions();

	}
	ImGui::SetNextWindowPos({ 0,0 });
	ImGui::SetNextWindowSize({
		(float)m_app->getWindow()->getWindowWidth(),
		(float)m_app->getWindow()->getWindowHeight()
	});
	if (ImGui::Begin("BACKGROUND", nullptr, m_backdrop)) {

	}
	ImGui::End();
	return false;
}

void InGameMenuState::renderMenu() {
	ImVec2 size = ImVec2(400.f, 0.f);
	ImVec2 pos = ImVec2(
		m_outerPadding,
		m_outerPadding
	);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::PushFont(m_imGuiHandler->getFont("Beb60"));

	if (ImGui::Begin("##Pause Menu", nullptr, m_standaloneButtonflags)) {
		if (SailImGui::TextButton("Close menu") || (!m_openedThisFrame && ImGui::IsKeyPressed(KeyBinds::SHOW_IN_GAME_MENU))) {
			Input::HideCursor(true);
			this->requestStackPop();
		}
		if (m_windowToRender == 1) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		if (SailImGui::TextButton((m_windowToRender == 1) ? ">Options" : "Options")) {
			if (m_windowToRender != 1) {
				m_windowToRender = 1;
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			else {
				m_windowToRender = 0;
				ImGui::PopStyleColor();
			}
		}
		if (m_windowToRender == 1) {
			ImGui::PopStyleColor();
		}
		if (SailImGui::TextButton("Exit to main menu")) {
			// Reset the network
			NWrapperSingleton::getInstance().resetNetwork();
			NWrapperSingleton::getInstance().resetWrapper();
			// Dispatch game over event
			EventDispatcher::Instance().emit(GameOverEvent());

			this->requestStackClear();
			this->requestStackPush(States::MainMenu);
		}
	}

	ImGui::End();
	ImGui::PopFont();

}

void InGameMenuState::renderOptions() {

	ImGui::SetNextWindowPos(m_pos);
	ImGui::SetNextWindowSize(m_size);
	ImGui::SetNextWindowSizeConstraints(m_minSize, m_maxSize);

	if (ImGui::Begin("##OptionWindow", NULL, m_backgroundOnlyflags)) {

		ImGui::PushFont(m_imGuiHandler->getFont("Beb40"));
		SailImGui::HeaderText("Options");
		ImGui::PopFont();
		ImGui::Separator();
		m_optionsWindow.renderWindow();
	}
	ImGui::End();



}

bool InGameMenuState::IsOpen() {
	return sIsOpen;
}
