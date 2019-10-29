#include "EndGameState.h"
#include "Sail/entities/ECS.h"
#include "Sail/Application.h"
#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include "Sail/entities/systems/Systems.h"
#include "Sail/KeyBinds.h"
#include "Sail/utils/GameDataTracker.h"
#include "Network/NWrapperSingleton.h"
#include "../libraries/imgui/imgui.h"

EndGameState::EndGameState(StateStack& stack) : State(stack) {

}

EndGameState::~EndGameState() {
	ECS::Instance()->stopAllSystems();
}

bool EndGameState::processInput(float dt) {
	return true;
}

bool EndGameState::update(float dt, float alpha) {
	NWrapperSingleton::getInstance().getNetworkWrapper()->checkForPackages();

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
				onReturnToLobby(NetworkBackToLobby{});
			}
		}
		if (ImGui::Button("Main menu")) {
			NWrapperSingleton::getInstance().resetNetwork();
			NWrapperSingleton::getInstance().resetWrapper();
			GameDataTracker::getInstance().resetData();
			this->requestStackPop();
			this->requestStackPush(States::MainMenu);
		}
		if (ImGui::Button("Quit")) {
			PostQuitMessage(0);
		}

	}
	ImGui::End();

	GameDataTracker::getInstance().renderImgui();

	return true;
}

bool EndGameState::onEvent(Event& event) {

	EventHandler::dispatch<NetworkBackToLobby>(event, SAIL_BIND_EVENT(&EndGameState::onReturnToLobby));
	EventHandler::dispatch<NetworkDisconnectEvent>(event, SAIL_BIND_EVENT(&EndGameState::onClientLeft));

	return true;
}

bool EndGameState::onReturnToLobby(NetworkBackToLobby& event) {
	// If host, propagate to other clients
	if (NWrapperSingleton::getInstance().isHost()) {
		// Send it all clients
		NWrapperSingleton::getInstance().getNetworkWrapper()->sendMsgAllClients("z");
		this->requestStackPop();
		this->requestStackPush(States::HostLobby);
	}
	else {
		this->requestStackPop();
		this->requestStackPush(States::JoinLobby);
	}
	GameDataTracker::getInstance().resetData();
	
	return true;
}

bool EndGameState::onClientLeft(NetworkDisconnectEvent& event) {
	Netcode::PlayerID id = event.getPlayerID();
	NWrapperSingleton::getInstance().playerLeft(id);


	return true;
}


