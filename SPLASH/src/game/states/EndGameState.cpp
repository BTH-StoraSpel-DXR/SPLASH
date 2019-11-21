#include "EndGameState.h"
#include "Sail/entities/ECS.h"
#include "Sail/Application.h"
#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include "Sail/entities/systems/Systems.h"
#include "Sail/KeyBinds.h"
#include "Sail/utils/GameDataTracker.h"
#include "Network/NWrapperSingleton.h"
#include "../libraries/imgui/imgui.h"
#include "Sail/events/EventDispatcher.h"
#include "../events/NetworkJoinedEvent.h"

EndGameState::EndGameState(StateStack& stack) : 
	State(stack),
	m_padding(30)
{
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_JOINED, this);

	m_app = Application::getInstance();
	m_imguiHandler = m_app->getImGuiHandler();

	if (NWrapperSingleton::getInstance().getPlayers().size() > 1) {
		NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::EndGame, 1);
	}

#ifdef DEVELOPMENT
	//KEEP FOR DEBUGGING
	//GameDataTracker::getInstance().init();
	//GameDataTracker::getInstance().addDebugData();
#endif
	m_standaloneButtonflags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBackground;



	m_backgroundOnlyflags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings;

}

EndGameState::~EndGameState() {
	ECS::Instance()->stopAllSystems();
	GameDataTracker::getInstance().resetData();

	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_JOINED, this);

	//Randomize Seed for the next Game
	if (NWrapperSingleton::getInstance().isHost() && m_app->getSettings().gameSettingsDynamic["map"]["keepSeed"].value == 0) {
		m_app->getSettings().gameSettingsDynamic["map"]["seed"].value = Utils::rnd() * m_app->getSettings().gameSettingsDynamic["map"]["seed"].maxVal;
		NWrapperSingleton::getInstance().getNetworkWrapper()->updateGameSettings(m_app->getSettings().serialize(m_app->getSettings().gameSettingsStatic, m_app->getSettings().gameSettingsDynamic));
	}
}

bool EndGameState::processInput(float dt) {
	return true;
}

bool EndGameState::update(float dt, float alpha) {
	NWrapperSingleton::getInstance().checkForPackages();

	return true;
}
bool EndGameState::fixedUpdate(float dt) {


	return true;
}

bool EndGameState::render(float dt, float alpha) {
	Application::getInstance()->getAPI()->clear({ 0.1f, 0.2f, 0.3f, 1.0f });

	// Call the empty draw function to just clear the screen until we have actual graphics.
	ECS::Instance()->getSystem<BeginEndFrameSystem>()->renderNothing();

	return true;
}

bool EndGameState::renderImgui(float dt) {

	ImGui::PushFont(m_imguiHandler->getFont("Beb20"));
	ImGui::PushFont(m_imguiHandler->getFont("Beb50"));
	renderMenu();

	ImGui::PopFont();
	renderScore();

	renderPersonalStats();
	renderFunStats();

	//WORK IN PROGRESS
	//renderWinners();

	ImGui::PopFont();

	return true;
}

bool EndGameState::onEvent(const Event& event) {
	State::onEvent(event);

	switch (event.type) {
		case Event::Type::NETWORK_JOINED:	onPlayerJoined((const NetworkJoinedEvent&)event); break;
	default: break;
	}

	return true;
}

bool EndGameState::onPlayerJoined(const NetworkJoinedEvent& event) {

	if (NWrapperSingleton::getInstance().isHost()) {
		NWrapperSingleton::getInstance().getNetworkWrapper()->setTeamOfPlayer(-1, event.player.id, false);
		NWrapperSingleton::getInstance().getNetworkWrapper()->setClientState(States::JoinLobby, event.player.id);
	}

	return true;
}

void EndGameState::renderMenu() {

	ImGui::SetNextWindowPos(ImVec2(m_padding, m_padding));
	ImGui::SetNextWindowSize(ImVec2(300,700));
	if (ImGui::Begin("##GameOverMenu", nullptr, m_standaloneButtonflags)) {
		if (NWrapperSingleton::getInstance().isHost()) {
			if (SailImGui::TextButton("Lobby")) {
				NWrapperSingleton::getInstance().getNetworkWrapper()->setClientState(States::JoinLobby);
				this->requestStackPop();
				this->requestStackPush(States::HostLobby);
			}
		}
		if (SailImGui::TextButton("Main menu")) {
			NWrapperSingleton::getInstance().resetNetwork();
			NWrapperSingleton::getInstance().resetWrapper();
			GameDataTracker::getInstance().resetData();
			this->requestStackPop();
			this->requestStackPush(States::MainMenu);
		}
		if (SailImGui::TextButton("Quit")) {
			PostQuitMessage(0);
		}
	}
	ImGui::End();
}

void EndGameState::renderScore() {
	static ImVec2 size(0, 0);
	static ImVec2 pos(0, m_padding);
	
	size.x = m_app->getWindow()->getWindowWidth() * 0.6f;
	pos.x = (m_app->getWindow()->getWindowWidth() - m_padding * 2) * 0.5f - size.x * 0.5f;

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowSizeConstraints(ImVec2(786,400),ImVec2(4000,4000));
	if (ImGui::Begin("##PLACEMENTS", nullptr, m_backgroundOnlyflags)) {
		GameDataTracker::getInstance().renderPlacement();
	}
	ImGui::End();
}


void EndGameState::renderPersonalStats() {
	static ImVec2 size(200, 700);
	static ImVec2 pos(0, m_padding);

	pos.x = (m_app->getWindow()->getWindowWidth() - m_padding) - size.x;

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("##PERSONALSTATS", nullptr, m_backgroundOnlyflags)) {
		GameDataTracker::getInstance().renderPersonalStats();
	}
	ImGui::End();
}

void EndGameState::renderFunStats() {

	static ImVec2 size(200, 200);
	static ImVec2 pos(0, m_padding);


	size.x = m_app->getWindow()->getWindowWidth() * 0.6f;
	pos.x = (m_app->getWindow()->getWindowWidth() - m_padding * 2) * 0.5f - size.x * 0.5f;
	pos.y = (m_app->getWindow()->getWindowHeight() - m_padding - size.y);

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowSizeConstraints(ImVec2(786, 200), ImVec2(4000, 4000));
	if (ImGui::Begin("##FUNSTATS", nullptr, m_backgroundOnlyflags)) {
		GameDataTracker::getInstance().renderFunStats();
	}
	ImGui::End();


}

void EndGameState::renderWinners() {
	//WORK IN PROGRESS
	static ImVec2 size(200, 200);
	static ImVec2 pos(0, m_padding);


	size.x = m_app->getWindow()->getWindowWidth() * 0.6f;
	pos.x = (m_app->getWindow()->getWindowWidth() - m_padding * 2) * 0.5f - size.x * 0.5f;
	pos.y = (m_app->getWindow()->getWindowHeight() - m_padding - size.y);

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowSizeConstraints(ImVec2(786, 200), ImVec2(4000, 4000));
	if (ImGui::Begin("##FUNSTATS", nullptr, m_backgroundOnlyflags)) {
		GameDataTracker::getInstance().renderWinners();
	}
	ImGui::End();


}

