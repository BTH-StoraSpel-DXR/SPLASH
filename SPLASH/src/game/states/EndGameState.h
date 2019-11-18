#pragma once

#include "Sail/states/State.h"

class EndGameState final : public State {
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
	bool onEvent(const Event& event) override;

private:
	void updatePerTickComponentSystems(float dt);
	void updatePerFrameComponentSystems(float dt, float alpha);
};
