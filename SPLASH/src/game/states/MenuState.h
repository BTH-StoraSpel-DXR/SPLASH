#pragma once

#include "Sail.h"
#include "Network/NWrapperSingleton.h"
#include "Network/NWrapperHost.h"
#include <string>
#include <list>
#include <ctime>
#include "Sail/utils/SailImGui/OptionsWindow.h"
#include <filesystem>

class NetworkLanHostFoundEvent;

class MenuState final : public State {
public:
	typedef std::unique_ptr<State> Ptr;

public:
	MenuState(StateStack& stack);
	virtual ~MenuState();

	// Process input for the state
	bool processInput(float dt);
	// Updates the state
	bool update(float dt, float alpha = 1.0f);
	// Renders the state
	bool render(float dt, float alpha = 1.0f);
	// Renders imgui
	bool renderImgui(float dt);
	// Sends events to the state
	bool onEvent(const Event& event) override;

private:
	const std::string loadPlayerName(const std::string& file);


private:
	Input* m_input = nullptr;
	// NetworkWrapper | NWrapperSingleton | NWrapperHost
	NWrapperSingleton* m_network = nullptr;
	Application* m_app = nullptr;
	SettingStorage* m_settings;
	ImGuiHandler* m_imGuiHandler;
	ImGuiWindowFlags m_standaloneButtonflags;
	ImGuiWindowFlags m_backgroundOnlyflags;
	OptionsWindow m_optionsWindow;
	// For ImGui Input
	std::string inputIP;
	
	// Other lobbies
	bool onLanHostFound(const NetworkLanHostFoundEvent& event);
	//void sortFoundLobbies();
	void removeDeadLobbies();		// Only works with sorted lobbies
	const int m_ipBufferSize = 64;
	char* m_ipBuffer;
	int m_sleepTime;

	bool m_refreshing = true;
	 

	struct FoundLobby {
		GameOnLanDescription gameDescription;
		std::string serverIdentifier;
		double duration = 20;
		void resetDuration() { duration = 20; }
	};
	double m_frameTick = 0.1;
	double udpChill = 5;
	double udpCounter = 0;
	std::vector<FoundLobby> m_foundLobbies;
	std::vector<std::string> m_newfoundLobbies;
	std::vector<std::filesystem::path> m_replaysFound;
	std::vector<std::filesystem::path> m_unsavedReplaysFound;

	void renderDebug();
	void renderRAM();

	int m_windowToRender;
	bool m_joiningLobby;
	float m_joinTimer;
	float m_joinThreshold;
	float m_outerPadding;
	float m_menuWidth;
	ImVec2 m_minSize;
	ImVec2 m_maxSize;
	ImVec2 m_size;
	ImVec2 m_pos;
	float m_percentage;
	bool m_usePercentage;


	void joinLobby(std::string& ip); 

	void renderMenu();
	void renderSingleplayer();
	void renderLobbyCreator();
	void renderServerBrowser();
	void renderProfile();
	void renderJoiningLobby();
	void renderOptions();
	void renderReplays();
	void renderCredits(float dt);

	void prepareReplay(std::string replayName);
	void updateSavedReplays();

#ifdef DEVELOPMENT
	void startSinglePlayer();
#endif
};

