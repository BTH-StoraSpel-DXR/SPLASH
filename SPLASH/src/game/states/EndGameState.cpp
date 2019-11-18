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

EndGameState::EndGameState(StateStack& stack) : State(stack) {}

EndGameState::~EndGameState() {
	ECS::Instance()->stopAllSystems();
	GameDataTracker::getInstance().resetData();
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

	if (ImGui::Begin("Game Over")) {
		ImGui::SetWindowPos({ 750,12 });
		ImGui::SetWindowSize({ 102,100 });
		if (NWrapperSingleton::getInstance().isHost()) {
			if (ImGui::Button("Lobby")) {

				NWrapperSingleton::getInstance().getNetworkWrapper()->setClientState(States::JoinLobby);
				this->requestStackPop();
				this->requestStackPush(States::HostLobby);

				ImGui::End();
				return true;
			}
		}
		if (ImGui::Button("Main menu")) {
			NWrapperSingleton::getInstance().resetNetwork();
			NWrapperSingleton::getInstance().resetWrapper();
			GameDataTracker::getInstance().resetData();
			this->requestStackPop();
			this->requestStackPush(States::MainMenu);

			ImGui::End();
			return true;
		}
		if (ImGui::Button("Quit")) {
			PostQuitMessage(0);

			ImGui::End();
			return true;
		}

	}
	ImGui::End();

	GameDataTracker::getInstance().renderImgui();

	return true;
}

bool EndGameState::onEvent(const Event& event) {
	State::onEvent(event);

	return true;
}

