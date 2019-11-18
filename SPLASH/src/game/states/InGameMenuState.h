#pragma once
#include "Sail.h"

#include "Sail/utils/SailImGui/InGameMenuWindow.h"
#include "Sail/utils/SailImGui/OptionsWindow.h"

class InGameMenuState final : public State {
public:
	explicit InGameMenuState(StateStack& stack);
	~InGameMenuState();

	// Process input for the state
	bool processInput(float dt);
	// Updates the state
	bool update(float dt, float alpha = 1.0f);

	bool fixedUpdate(float dt);
	// Renders the state
	bool render(float dt, float alpha = 1.0f);
	// Renders imgui
	bool renderImgui(float dt);
	// Sends events to the state
	bool onEvent(Event& event) { return true; }
	

	static bool IsOpen();

private:
	Application* m_app;
	ImGuiHandler* m_imGuiHandler;
	static bool sIsOpen;
	float m_outerPadding;
	InGameMenuWindow m_inGameMenuWindow;
	OptionsWindow m_optionsWindow;
	bool m_isSinglePlayer;
	bool m_openedThisFrame;
	int m_windowToRender;

	ImGuiWindowFlags m_standaloneButtonflags;
	ImGuiWindowFlags m_backdrop;
	ImGuiWindowFlags m_backgroundOnlyflags;

	float m_menuWidth;
	ImVec2 m_minSize;
	ImVec2 m_maxSize;
	ImVec2 m_size;
	ImVec2 m_pos;
	float m_percentage;
	bool m_usePercentage;


	void renderMenu();
	void renderOptions();


};
