#pragma once
#include "Sail/states/State.h"
#include "Sail/graphics/Scene.h"
#include "Network/NetworkStructs.hpp"
#include "Sail/events/Event.h"

class Input;
class Application;
class NetworkWrapper;

class MenuState : public State, public NetworkEvent {

public:
	typedef std::unique_ptr<State> Ptr;

public:
	MenuState(StateStack& stack);
	virtual ~MenuState();

	// Process input for the state
	bool processInput(float dt);
	// Updates the state
	bool update(float dt);
	// Renders the state
	bool render(float dt);
	// Renders imgui
	bool renderImgui(float dt);
	// Sends events to the state
	bool onEvent(Event& event) { return true; }

private:
	Input* m_input = nullptr;
	NetworkWrapper* m_network = nullptr;
	Application* m_app = nullptr;
	// For ImGui Input
	char* inputIP = nullptr;
	char* inputName = nullptr;
	Scene m_scene;
};