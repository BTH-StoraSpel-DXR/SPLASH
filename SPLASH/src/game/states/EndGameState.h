#pragma once

#include "Sail/states/State.h"
#include "../SPLASH/src/game/events/NetworkBackToLobby.h"
#include "../SPLASH/src/game/events/NetworkDisconnectEvent.h"

class EndGameState : public State {
public:
	explicit EndGameState(StateStack& stack);
	~EndGameState();

	// Process input for the state
	bool processInput(float dt) override;
	// Updates the state
	bool update(float dt, float alpha = 1.0f) override;
	bool fixedUpdate(float dt) override;
	// Renders the state
	bool render(float dt, float alpha = 1.0f) override;
	// Renders imgui
	bool renderImgui(float dt) override;
	// Sends events to the state
	bool onEvent(Event& event) override;

private:
	bool onReturnToLobby(NetworkBackToLobby& event);
	bool onClientLeft(NetworkDisconnectEvent& event);

	void updatePerTickComponentSystems(float dt);
	void updatePerFrameComponentSystems(float dt, float alpha);



};
