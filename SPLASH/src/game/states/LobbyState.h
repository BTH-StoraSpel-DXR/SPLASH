#pragma once

#include "Sail.h"
#include <string>
#include <list>
#include "Sail/utils/SailImGui/OptionsWindow.h"
#include "Sail/utils/SailImGui/NetworkInfoWindow.h"

struct Player;
class NWrapper;
struct TextInputEvent;
class AudioComponent;

struct ChatSent;
struct NetworkChatEvent;
struct NetworkJoinedEvent;
struct NetworkDisconnectEvent;
struct NetworkPlayerChangedTeam;
struct NetworkPlayerRequestedTeamChange;
struct NetworkTeamColorRequest;

#define HOST_ID 0





class LobbyState : public State {
public:
	LobbyState(StateStack& stack);
	virtual ~LobbyState();

	// Process input for the state
	bool processInput(float dt);
	// Updates the state
	virtual bool update(float dt, float alpha = 1.0f);
	// Renders the state
	bool render(float dt, float alpha = 1.0f);
	// Renders imgui
	bool renderImgui(float dt);
	// Sends events to the state

protected:
	Application* m_app = nullptr;
	Input* m_input = nullptr;
	NWrapper* m_network = nullptr;
	SettingStorage* m_settings;
	int* m_settingBotCount = nullptr;

	virtual bool onEvent(const Event& event) override;

private:
	ImGuiHandler* m_imGuiHandler;
	OptionsWindow m_optionsWindow;
	NetworkInfoWindow m_netInfo;
	bool m_ready;
	// LobbyAudio
	Entity* m_lobbyAudio = nullptr;

	unsigned int m_tempID = 0; // used as id counter until id's are gotten through network shit.
	
	// Render ImGui Stuff --------- WILL BE REPLACED BY OTHER GRAPHICS.
	bool m_settingsChanged;
	float m_timeSinceLastUpdate;

	ImGuiWindowFlags m_standaloneButtonflags;
	ImGuiWindowFlags m_backgroundOnlyflags;


	int m_windowToRender;
	unsigned int m_outerPadding;

	bool m_renderGameSettings;
	bool m_renderApplicationSettings;

	float m_screenWidth;
	float m_screenHeight;

	float m_menuWidth;
	ImVec2 m_minSize;
	ImVec2 m_maxSize;
	ImVec2 m_size;
	ImVec2 m_pos;
	float m_percentage;
	bool m_usePercentage;


	bool onPlayerJoined(const NetworkJoinedEvent& event);
	bool onPlayerDisconnected(const NetworkDisconnectEvent& event);
	bool onPlayerTeamRequest(const NetworkPlayerRequestedTeamChange& event);
	bool onTeamColorRequest(const NetworkTeamColorRequest& event);
	bool onPlayerTeamChanged(const NetworkPlayerChangedTeam& event);
	bool onSettingsChanged();

	void renderPlayerList();
	void renderGameSettings();		// Currently empty
	void renderMenu();
};