#pragma once

#include "Sail.h"
#include "Network/NWrapperSingleton.h"
#include "Network/NWrapperHost.h"
#include <string>
#include <list>
#include <ctime>
class SplashScreenState : public State {
public:
	typedef std::unique_ptr<State> Ptr;

public:
	SplashScreenState(StateStack& stack);
	virtual ~SplashScreenState();

	// Process input for the state
	bool processInput(float dt);
	// Updates the state
	bool update(float dt, float alpha = 1.0f);
	// Renders the state
	bool render(float dt, float alpha = 1.0f);
	// Renders imgui
	bool renderImgui(float dt);
	// Sends events to the state
	virtual bool onEvent(const Event& event) override;

private:
	bool loadModels(Application* app);
	bool loadTextures(Application* app);


private:
	Input* m_input = nullptr;
	Application* m_app = nullptr;
	std::future<bool> m_modelThread;
};

