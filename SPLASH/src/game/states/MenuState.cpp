#include "Sail/../../Sail/src/Network/NetworkModule.hpp"
#include "MenuState.h"

#include <Psapi.h>

#include "Sail.h"
#include "../libraries/imgui/imgui.h"

#include "Network/NWrapperSingleton.h"
#include "Network/NWrapper.h"

#include "Sail/events/EventDispatcher.h"
#include "Sail/../../SPLASH/src/game/events/NetworkLanHostFoundEvent.h"

#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include <string>
#include "Sail/utils/SailImGui/SailImGui.h"

// Mainly used in progress bar
#define TOTAL_NR_OF_MODELS 25
#define TOTAL_NR_OF_TEXTURES 88


MenuState::MenuState(StateStack& stack) 
	: State(stack),
	inputIP("") {
	m_input = Input::GetInstance();
	m_network = &NWrapperSingleton::getInstance();
	m_app = Application::getInstance();
	m_imGuiHandler = m_app->getImGuiHandler();
	m_settings = &m_app->getSettings();
	std::string name = loadPlayerName("res/data/localplayer.settings");
	m_network->setPlayerName(name.c_str());
	
	m_ipBuffer = SAIL_NEW char[m_ipBufferSize];

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


	m_optionsWindow.setPosition({ 300, 300 });
	m_windowToRender = 0;
	m_joiningLobby = false;
	m_joinTimer = 0.0f;
	m_joinThreshold = 10.0f;
	m_outerPadding = 35;
	m_menuWidth = 700.0f;
	m_usePercentage = true;
	m_percentage = 0.45f;

	m_pos = ImVec2(0, 0);
	m_size = ImVec2(0, 0);
	m_minSize = ImVec2(435, 500);
	m_maxSize = ImVec2(1000, 1000);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_LAN_HOST_FOUND, this);
}

MenuState::~MenuState() {
	delete[] m_ipBuffer;
	
	Utils::writeFileTrunc("res/data/localplayer.settings", NWrapperSingleton::getInstance().getMyPlayerName());

	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_LAN_HOST_FOUND, this);
}

bool MenuState::processInput(float dt) {
	return false;
}

bool MenuState::update(float dt, float alpha) {
	udpCounter -= m_frameTick;
	if (udpCounter < 0) {
		NWrapperSingleton::getInstance().searchForLobbies();
		udpCounter = udpChill;
	}
	NWrapperSingleton::getInstance().checkForPackages();


	if (m_joiningLobby) {
		m_joinTimer += dt;
		if (m_joinTimer > m_joinThreshold) {
			m_joiningLobby = false;
			m_joinTimer = 0.0f;
		}
	}

	removeDeadLobbies();
	return false;
}

bool MenuState::render(float dt, float alpha) {
	m_app->getAPI()->clear({ 0.1f, 0.2f, 0.3f, 1.0f });
	ECS::Instance()->getSystem<BeginEndFrameSystem>()->renderNothing();
	return false;
}

bool MenuState::renderImgui(float dt) {

	// Showcasing imgui texture rendering - remove when everyone who needs to know how this works knows how this works.
	auto* imguiHandler = Application::getInstance()->getImGuiHandler();




	
#ifdef DEVELOPMENT
	//Keep
	ImGui::ShowDemoWindow();
#endif
if (m_usePercentage) {
		m_menuWidth = m_percentage * m_app->getWindow()->getWindowWidth();
	}

	m_size.x = m_menuWidth;
	m_size.y = m_app->getWindow()->getWindowHeight() - m_outerPadding * 2;
	m_pos.x = m_app->getWindow()->getWindowWidth() - m_outerPadding - ((m_size.x < m_minSize.x) ? m_minSize.x : m_size.x);
	m_pos.y = m_outerPadding;

	

	static std::string font = "Beb24";

#ifdef DEVELOPMENT
	ImGui::PushFont(m_imGuiHandler->getFont(font));
	if (ImGui::Begin("IMGUISETTINGS")) {
		if (ImGui::BeginCombo("##FONTS", &font.front())) {
			for (auto const& [key, val] : m_imGuiHandler->getFontMap()) {
				ImGui::PushFont(val);

				if(ImGui::Selectable(key.c_str(), font == key)) {
					font = key;
				}
				ImGui::PopFont();
			}
			ImGui::EndCombo();
		}
		ImGui::SliderFloat("Width##rightwindow", &m_menuWidth, 0, 1920);
		ImGui::Checkbox("percentage", &m_usePercentage);
		if (m_usePercentage) {
			ImGui::SameLine();
			ImGui::SliderFloat("percentage##rightwindow", &m_percentage, 0, 1);
		}
	}
	ImGui::End();

	ImGui::PopFont();
#endif
	ImGui::PushFont(m_imGuiHandler->getFont(font));


	renderMenu();

	ImVec4 col(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	col.w = 0.9;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, col);

	// NOT YET IMPLEMENTED
	renderSingleplayer();


	if (m_windowToRender == 1) {
		renderLobbyCreator();
	}
	if (m_windowToRender == 2) {
		renderServerBrowser();
	}
	if (m_windowToRender == 3) {
		renderOptions();
	}
	if (m_windowToRender == 4) {
		renderProfile();
	}
#ifdef DEVELOPMENT
	renderDebug();
	renderRAM();
#endif
	if (m_joiningLobby) {
		renderJoiningLobby();
	}
	ImGui::PopStyleColor();

	if (ImGui::Begin("##LOGOWINDOW", nullptr, m_standaloneButtonflags)) {
		static ImVec2 z(3.7, 1.0);
		Texture& logo = m_app->getResourceManager().getTexture("splash_logo_smaller.tga");
		ImGui::Image(imguiHandler->getTextureID(&logo), ImVec2(m_app->getWindow()->getWindowWidth() *0.4f, (m_app->getWindow()->getWindowWidth()*0.4f) / z.x));
		ImGui::SetWindowPos(ImVec2(m_app->getWindow()->getWindowWidth()*0.5f - ImGui::GetWindowSize().x*0.5f, 0));
	}
	ImGui::End();

	ImGui::PopFont();


	return false;
}

bool MenuState::onEvent(const Event& event) {
	State::onEvent(event);
	switch (event.type) {
	case Event::Type::NETWORK_LAN_HOST_FOUND: onLanHostFound((const NetworkLanHostFoundEvent&)event); break;
	default: break;
	}

	return true;
}

const std::string MenuState::loadPlayerName(const std::string& file) {
	std::string name = Utils::readFile(file);
	if (name == "") {
		name = "Hans";
		Utils::writeFileTrunc("res/data/localplayer.settings", name);
		SAIL_LOG("Found no player file, created: " + std::string("'res/data/localplayer.settings'"));
	}
	return name;
}

bool MenuState::onLanHostFound(const NetworkLanHostFoundEvent& event) {
	// Get IP (as int) then convert into char*
	std::string serverIdentifier = event.gameDescription.ip + ":" + std::to_string(event.gameDescription.port);

	// Check if it is already logged.	
	bool alreadyExists = false;
	for (auto& lobby : m_foundLobbies) {
		if (lobby.serverIdentifier == serverIdentifier) {
			alreadyExists = true;
			lobby.gameDescription = event.gameDescription;
			lobby.resetDuration();
		}
	}
	// If not...
	if (alreadyExists == false) {
		// ...log it.
		m_foundLobbies.push_back(FoundLobby{ event.gameDescription, serverIdentifier });
	}

	return false;
}

void MenuState::removeDeadLobbies() {
	// How much time passed since last time this function was called?
	std::vector<FoundLobby> lobbiesToRemove;

	// Find out which lobbies should be removed
	for (auto& lobby : m_foundLobbies) {
		lobby.duration -= m_frameTick;

		// Remove them based on UDP inactivity
		if (lobby.duration < 0) {
			// Queue the removal
			lobbiesToRemove.push_back(lobby);
		}
	}

	// Remove queued lobbies.
	int index;
	for (auto& lobbyToRemove : lobbiesToRemove)	{
		index = 0;

		for (auto& lobby : m_foundLobbies) {
			if (lobbyToRemove.serverIdentifier == lobby.serverIdentifier) {
				m_foundLobbies.erase(m_foundLobbies.begin()+index);
				break;
			}
			index++;
		}
	}
}

void MenuState::renderDebug() {

	if (ImGui::Begin("Loading Info")) {
		auto& rm = m_app->getResourceManager();

		std::string progress = "Models:";
		ImGui::Text(progress.c_str());
		ImGui::SameLine();
		ImGui::ProgressBar(
			rm.numberOfModels() / (float)TOTAL_NR_OF_MODELS,
			ImVec2(0.0f, 0.0f),
			std::string(std::to_string(rm.numberOfModels()) + ":" + std::to_string(TOTAL_NR_OF_MODELS)).c_str()
		);

		progress = "Textures:";
		ImGui::Text(progress.c_str());
		ImGui::SameLine();
		ImGui::ProgressBar(
			rm.numberOfTextures() / (float)TOTAL_NR_OF_TEXTURES,
			ImVec2(0.0f, 0.0f),
			std::string(std::to_string(rm.numberOfTextures()) + ":" + std::to_string(TOTAL_NR_OF_TEXTURES)).c_str()
		);
	}
	ImGui::End();
}

void MenuState::renderRAM() {
	static int framesElapsed = 100;		// Trigger the update on the first frame
	framesElapsed++;

	static size_t physicalMemory = 0;
	auto workSetUsage = []() {
		PROCESS_MEMORY_COUNTERS_EX pmc;
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		return pmc.WorkingSetSize;
	};

	if (framesElapsed > 100) {
		physicalMemory = workSetUsage();
		framesElapsed = 0;
	}

	if (ImGui::Begin("RAM usage")) {
		auto str = std::to_string(physicalMemory / (1024 * 1024)) + " MB";
		ImGui::Text(str.c_str());
	}
	ImGui::End();
}

void MenuState::joinLobby(std::string& ip) {
	//KEEP
	//m_joinTimer = 0.0f;
	//m_joiningLobby = true;

	if (m_network->connectToIP(&ip.front())) {
	}
}

void MenuState::renderMenu() {
	ImGui::PushFont(m_imGuiHandler->getFont("Beb60"));

	if (ImGui::Begin("##MAINMENU", nullptr, m_standaloneButtonflags)) {
#ifdef DEVELOPMENT
		if (SailImGui::TextButton("Singleplayer  #dev")) {
			startSinglePlayer();
		}
#endif
		if (m_windowToRender == 1) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		if (SailImGui::TextButton((m_windowToRender == 1) ? ">Create lobby" : "Create Lobby")) {
			if (m_windowToRender != 1) {
				m_windowToRender = 1;
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			else {
				m_windowToRender = 0;
				ImGui::PopStyleColor();
			}
		}
		if (m_windowToRender == 1) {
			ImGui::PopStyleColor();
		}




		if (m_windowToRender == 2) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		if (SailImGui::TextButton((m_windowToRender == 2) ? ">Server browser" : "Server browser")) {
			if (m_windowToRender != 2) {
				m_windowToRender = 2;
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			else {
				m_windowToRender = 0;
				ImGui::PopStyleColor();
			}
		}
		if (m_windowToRender == 2) {
			ImGui::PopStyleColor();
		}
		if (m_windowToRender == 3) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		if (SailImGui::TextButton((m_windowToRender == 3) ? ">Options" : "Options")) {
			if (m_windowToRender != 3) {
				m_windowToRender = 3;
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			else {
				m_windowToRender = 0;
				ImGui::PopStyleColor();
			}
		}
		if (m_windowToRender == 3) {
			ImGui::PopStyleColor();
		}

		if (m_windowToRender == 4) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}

		std::string profileText = (m_windowToRender == 4 ? ">Profile" : "Profile");
		profileText += " (" + m_network->getMyPlayerName() + ")";

		if (SailImGui::TextButton(profileText.c_str())){
			if (m_windowToRender != 4) {
				m_windowToRender = 4;
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			else {
				m_windowToRender = 0;
				ImGui::PopStyleColor();
			}
		}
		if (m_windowToRender == 4) {
			ImGui::PopStyleColor();
		}
#ifdef DEVELOPMENT
		if (SailImGui::TextButton("Pause  #dev")) {
			this->requestStackPush(States::InGameMenu);
		}
#endif

		if (SailImGui::TextButton("Quit")) {
			PostQuitMessage(0);
		}
	}
	ImGui::End();

	ImGui::PopFont();
}

void MenuState::renderSingleplayer() {
}

void MenuState::renderLobbyCreator() {


	static char buf[101] = "";
	static std::string lobbyName = "";
	std::string name = NWrapperSingleton::getInstance().getMyPlayerName();

	ImGui::SetNextWindowPos(m_pos);
	ImGui::SetNextWindowSize(m_size);
	ImGui::SetNextWindowSizeConstraints(m_minSize, m_maxSize);
	if (ImGui::Begin("##HOSTLOBBY", nullptr, m_backgroundOnlyflags)) {
		ImGui::PushFont(m_imGuiHandler->getFont("Beb40"));
		SailImGui::HeaderText("Lobby Creator");
		ImGui::PopFont();
		ImGui::Separator();




		strncpy_s(buf, lobbyName.c_str(), lobbyName.size());
		ImGui::Text("Lobby Name: ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX());
		ImGui::InputTextWithHint("##lobbyName", std::string(name + "'s lobby").c_str(), buf, 40);
		lobbyName = buf;
		ImGui::Separator();
		if (ImGui::BeginChild("##LOBBYOPTIONS", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 6))) {
			// TODO: options


			if (m_optionsWindow.renderGameOptions()) {
			
			}



		}
		ImGui::EndChild();



		ImGui::Separator();
		if (ImGui::Button("Host Game")) {
			if (m_network->host()) {
				// Update server description after host added himself to the player list.
				NWrapperSingleton::getInstance().getMyPlayer().id = HOST_ID;
				NWrapperSingleton::getInstance().playerJoined(NWrapperSingleton::getInstance().getMyPlayer());
				NWrapperHost* wrapper = static_cast<NWrapperHost*>(NWrapperSingleton::getInstance().getNetworkWrapper());
				if (lobbyName == "") {
					lobbyName = NWrapperSingleton::getInstance().getMyPlayer().name + "'s lobby";
				}

				std::string gamemode = m_settings->gameSettingsStatic["gamemode"]["types"].getSelected().name;
				std::string map = m_settings->defaultMaps[gamemode].getSelected().name;
				wrapper->setLobbyName(lobbyName+";"+gamemode+";"+map);

				this->requestStackPop();
				this->requestStackPush(States::HostLobby);
			}
		}

	}
	ImGui::End();
	

}

void MenuState::renderServerBrowser() {
	// Display open lobbies
	const static std::string windowName = "Server browser";
	static char buf[101] = "";
	static std::string lobbyName = "";
	std::string name = NWrapperSingleton::getInstance().getMyPlayerName();


	ImGui::SetNextWindowPos(m_pos);
	ImGui::SetNextWindowSize(m_size);
	ImGui::SetNextWindowSizeConstraints(m_minSize, m_maxSize);
	if (ImGui::Begin("##Hosted Lobbies on LAN", nullptr, m_backgroundOnlyflags)) {
		ImGui::PushFont(m_imGuiHandler->getFont("Beb40"));
		SailImGui::HeaderText(windowName.c_str());
		ImGui::PopFont();
		ImGui::Separator();

		strncpy_s(buf, inputIP.c_str(), inputIP.size());
		ImGui::InputTextWithHint("##IP:", "127.0.0.1:54000", buf, 100);
		inputIP = buf;
		ImGui::SameLine();
		if (ImGui::Button("Join##ip")) {
			if (inputIP == "") {
				inputIP = "127.0.0.1:54000";
			}
			joinLobby(inputIP);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();

			ImGui::Text("Leave empty to join local game");
			ImGui::EndTooltip();
		}
		// DISPLAY LOBBIES
		static int selected = -1;
#ifdef DEVELOPMENT
		//KEEP
		//ImGui::Text(std::to_string(selected).c_str());
#endif
		if (ImGui::BeginChild("Lobbies", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
			// Per hosted game

			static float p[4] = { 0.24,0.46,0.65,0.4};
#ifdef DEVELOPMENT
			ImGui::SliderFloat4("asd", &p[0], 0.0f, 1.0f);
#endif
			ImGui::Columns(5, "serverbrowserColumns", false);
			ImGui::SetColumnOffset(1,ImGui::GetWindowContentRegionWidth() * p[0]);
			ImGui::SetColumnOffset(2,ImGui::GetWindowContentRegionWidth() * p[1]);
			ImGui::SetColumnOffset(3,ImGui::GetWindowContentRegionWidth() * p[2]);
			ImGui::SetColumnOffset(4,ImGui::GetWindowContentRegionWidth()-100);
			ImGui::Separator();
			ImGui::Text("Lobby"); ImGui::NextColumn();
			ImGui::Text("gamemode"); ImGui::NextColumn();
			ImGui::Text("map"); ImGui::NextColumn();
			ImGui::Text("Status"); ImGui::NextColumn();
			ImGui::Text("Players"); ImGui::NextColumn();
			ImGui::Separator();

			int index = 0;
			if (selected >= m_foundLobbies.size()) {
				selected = -1;
			}
			for (auto& lobby : m_foundLobbies) {
				// List as a button

				int first = lobby.gameDescription.name.find_first_of(";");
				int last = lobby.gameDescription.name.find_last_of(";");

				std::string lobbyName = lobby.gameDescription.name.substr(0,first);
				std::string map = lobby.gameDescription.name.substr(last+1, std::string::npos);
				std::string gamemode = lobby.gameDescription.name.substr(first + 1, last - first-1);

				std::string playerCount = (lobby.gameDescription.maxPlayers > 0) ? std::to_string(lobby.gameDescription.nPlayers) + " / " + std::to_string(lobby.gameDescription.maxPlayers) : "N/A";



				std::string statusString = "Unknown";

				if (lobby.gameDescription.currentState == States::Lobby) {
					statusString = "In Lobby";
				} else if (lobby.gameDescription.currentState == States::Game) {
					statusString = "In Game";
				} else if (lobby.gameDescription.currentState == States::EndGame) {
					statusString = "Endgame";
				}

				// GET LOBBY NAME AND PLAYERCOUNT AS SEPARATE STRINGS
				if (lobbyName == "") {
					lobbyName = lobby.serverIdentifier;
				}

				if (ImGui::Selectable(lobbyName.c_str(), selected == index, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {

					selected = (index == selected ? -1 : index);
					if (ImGui::IsMouseDoubleClicked(0)) {
						// If pressed then join
						joinLobby(lobby.serverIdentifier);
					}
				}
				ImGui::NextColumn();
				ImGui::Text(gamemode.c_str()); ImGui::NextColumn();
				ImGui::Text(map.c_str()); ImGui::NextColumn();
				ImGui::Text(statusString.c_str()); ImGui::NextColumn();



				ImGui::Text(playerCount.c_str()); ImGui::NextColumn();
				index++;
			}
		}
		ImGui::EndChild();

		// DISPLAY JOIN BUTTON
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 100);
		if (selected == -1) {
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.3f);
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		}
		// If pressed then join
		if (ImGui::Button("Join##browser") && selected > -1) {
			joinLobby(m_foundLobbies[selected].serverIdentifier);
		}
		if (selected == -1) {
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();

			ImGui::Text("Join Selected game");
			ImGui::EndTooltip();
		}
		
	}
	ImGui::End();

}
void MenuState::renderProfile() {
	std::string name = &NWrapperSingleton::getInstance().getMyPlayerName().front();

	const static std::string windowName = "Profile";
	ImGui::SetNextWindowPos(m_pos);
	ImGui::SetNextWindowSize(m_size);
	ImGui::SetNextWindowSizeConstraints(m_minSize, m_maxSize);

	if (ImGui::Begin("##Pause Menu", NULL, m_backgroundOnlyflags)) {
		ImGui::PushFont(m_imGuiHandler->getFont("Beb40"));
		SailImGui::HeaderText(windowName.c_str());
		ImGui::PopFont();
		ImGui::Separator();

		float innerWidth = ImGui::GetWindowContentRegionWidth();

		ImGui::Text("Name: ");
		ImGui::SameLine();
		static char buf[101] = "";
		strncpy_s(buf, name.c_str(), name.size());
		if (ImGui::InputText("##name", buf, MAX_NAME_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue)) {
			name = buf;
			while (name.find("%") != std::string::npos) {
				name = name.erase(name.find("%"), 1);
			}

			if (name == "") {
				name = "Hans";
			}

			NWrapperSingleton::getInstance().setPlayerName(name.c_str());
		}
		else {
			name = buf;
		}



		ImGui::Separator();
		//float x[3] = { 0.4f, 0.6f, 0.9 };
		//ImGui::Text(std::string("Extinguishes: ").c_str());
		//ImGui::SameLine(innerWidth * x[0]);
		//ImGui::Text(std::to_string(0).c_str());
		//ImGui::SameLine(innerWidth * x[1]);
		//ImGui::Text(std::string("lunacies: ").c_str());
		//ImGui::SameLine(innerWidth * x[2]);
		//ImGui::Text(std::to_string(0).c_str());
		//
		//ImGui::Text(std::string("Wins: ").c_str());
		//ImGui::SameLine(innerWidth * x[0]);
		//ImGui::Text(std::to_string(0).c_str());
		//ImGui::SameLine(innerWidth * x[1]);
		//ImGui::Text(std::string("Losses: ").c_str());
		//ImGui::SameLine(innerWidth * x[2]);
		//ImGui::Text(std::to_string(0).c_str());





		ImGui::End();
	}
}

void MenuState::renderJoiningLobby() {
	ImVec2 screenMiddle(
		m_app->getWindow()->getWindowWidth() * 0.5f, 
		m_app->getWindow()->getWindowHeight() * 0.5f
	);
	ImVec2 size((float)m_app->getWindow()->getWindowWidth(), (float)m_app->getWindow()->getWindowHeight());
	ImVec2 pos(0,0);

	ImGui::SetNextWindowBgAlpha(0.5f);
	ImGui::SetNextWindowPos({0.0f, 0.0f});
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("##JOININGWINDOWBG", nullptr, m_backgroundOnlyflags)) {
		ImVec2 innerSize(300, 300);
		ImGui::SetNextWindowPos({
			screenMiddle.x - innerSize.x* 0.5f,
			screenMiddle.y - innerSize.y* 0.5f
			});
		ImGui::SetNextWindowSize(innerSize);
		if (ImGui::Begin("##JOININGWINDOW", nullptr, m_backgroundOnlyflags)) {
			ImGui::Text("ASDASDKLJASLJKDLKJASLKDJASDALSKD");
			ImGui::Text("ASDASDKLJASLJKDLKJASLKDJASDALSKD");
			ImGui::Text("ASDASDKLJASLJKDLKJASLKDJASDALSKD");
			ImGui::Text("ASDASDKLJASLJKDLKJASLKDJASDALSKD");
			ImGui::Text("ASDASDKLJASLJKDLKJASLKDJASDALSKD");
			ImGui::Text("ASDASDKLJASLJKDLKJASLKDJASDALSKD");
		}
		ImGui::End();
	}
	ImGui::End();
}
void MenuState::renderOptions() {


	ImGui::SetNextWindowPos(m_pos);
	ImGui::SetNextWindowSize(m_size);
	ImGui::SetNextWindowSizeConstraints(m_minSize, m_maxSize);

	if (ImGui::Begin("##Pause Menu", NULL, m_backgroundOnlyflags)) {

		ImGui::PushFont(m_imGuiHandler->getFont("Beb40"));
		SailImGui::HeaderText("Options");
		ImGui::PopFont();
		ImGui::Separator();
		m_optionsWindow.renderWindow();
	}
	ImGui::End();
}
#ifdef DEVELOPMENT
void MenuState::startSinglePlayer() {

	if (m_network->host()) {
		NWrapperSingleton::getInstance().setPlayerID(HOST_ID);
		if (NWrapperSingleton::getInstance().getPlayers().size() == 0) {
			NWrapperSingleton::getInstance().playerJoined(NWrapperSingleton::getInstance().getMyPlayer());
		}
		NWrapperSingleton::getInstance().stopUDP();

		auto& map = m_app->getSettings().gameSettingsDynamic["map"];

#ifdef _PERFORMANCE_TEST
		map["sizeX"].value = 20;
		map["sizeY"].value = 20;
		map["seed"].value = 2.0f;
#endif


		this->requestStackClear();
		this->requestStackPush(States::Game);
	}

}
#endif