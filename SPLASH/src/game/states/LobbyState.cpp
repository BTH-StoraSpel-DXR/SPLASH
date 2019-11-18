#include "LobbyState.h"
#include "imgui_internal.h"
//#include "../imgui-sfml-master/imgui-SFML.h"
#include "../libraries/imgui/imgui.h"
#include "../Sail/src/API/DX12/imgui/DX12ImGuiHandler.h"

#include "../SPLASH/src/game/events/TextInputEvent.h"
#include "../SPLASH/src/game/events/NetworkChatEvent.h"
#include "../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "../SPLASH/src/game/events/NetworkDisconnectEvent.h"

#include "Sail/events/types/NetworkPlayerChangedTeam.h"
#include "Sail/events/types/NetworkPlayerRequestedTeamChange.h"

#include "Network/NWrapperSingleton.h"
#include "Network/NWrapper.h"
#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/Audio/AudioSystem.h"
#include "Sail/entities/components/AudioComponent.h"
#include "Sail/utils/Utils.h"
#include "Sail/utils/SailImGui/SailImGui.h"

LobbyState::LobbyState(StateStack& stack)
	: State(stack),
	m_settingsChanged(false),
	m_renderGameSettings(false),
	m_renderApplicationSettings(false),
	m_timeSinceLastUpdate(0.0f)
{
	NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::Lobby, 0);

	// ImGui is already initiated and set up, thanks alex!
	m_app = Application::getInstance();
	m_input = Input::GetInstance();
	m_network = NWrapperSingleton::getInstance().getNetworkWrapper();
	m_imGuiHandler = m_app->getImGuiHandler();
	m_settings = &m_app->getSettings();
	m_optionsWindow.setPosition({ 300, 300 });

	m_textHeight = 52;
	m_outerPadding = 15;

	m_messageLimit = 14;		// Should be based on size of chatbox instead of fixed
	m_messageCount = 0;
	m_settingBotCount = new int;
	*m_settingBotCount = 0;

	m_messageSizeLimit = 50;
	m_currentmessageIndex = 0;
	m_currentmessage = SAIL_NEW char[m_messageSizeLimit] { 0 };

	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_CHAT, this);
	EventDispatcher::Instance().subscribe(Event::Type::TEXTINPUT, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_JOINED, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_DISCONNECT, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_PLAYER_REQUESTED_TEAM_CHANGE, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_PLAYER_CHANGED_TEAM, this);

	m_ready = false;

	m_settingsChanged = false;
	m_timeSinceLastUpdate = 0.0f;


	m_menuWidth = 700.0f;
	m_usePercentage = true;
	m_percentage = 0.35f;

	m_pos = ImVec2(0, 0);
	m_size = ImVec2(0, 0);
	m_minSize = ImVec2(435, 500);
	m_maxSize = ImVec2(1000, 1000);

	m_windowToRender = 0;
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


	// TO DO: Streaming sounds in menu doesn't work because ESC is NOT UPDATED HERE
	//m_lobbyAudio = ECS::Instance()->createEntity("LobbyAudio").get();
	//m_lobbyAudio->addComponent<AudioComponent>();
	//m_lobbyAudio->getComponent<AudioComponent>()->streamSoundRequest_HELPERFUNC("res/sounds/LobbyMusic.xwb", true, true);

	if (NWrapperSingleton::getInstance().isHost()) {
		NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::Lobby, 1);
	}

}

LobbyState::~LobbyState() {
	delete[] m_currentmessage;
	delete m_settingBotCount;

	EventDispatcher::Instance().unsubscribe(Event::Type::TEXTINPUT, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_CHAT, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_JOINED, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DISCONNECT, this);

	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_PLAYER_REQUESTED_TEAM_CHANGE, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_PLAYER_CHANGED_TEAM, this);
}

bool LobbyState::processInput(float dt) {
	if (m_input->IsMouseButtonPressed(0)) {
		m_chatFocus = false;
	}

	return false;
}

bool LobbyState::inputToChatLog(const MSG& msg) {
	if (m_currentmessageIndex < m_messageSizeLimit && msg.wParam != KeyBinds::SEND_MESSAGE) {
		// Add whichever button that was inputted to the current message
		// --- OBS : doesn't account for capslock, etc.
		m_currentmessage[m_currentmessageIndex++] = (char)msg.wParam;
	}
	if (msg.wParam == KeyBinds::SEND_MESSAGE && m_chatFocus == false) {
		return true;
	}
	return false;
}

bool LobbyState::update(float dt, float alpha) {
	// Update screen dimensions & ImGui related
	// (Sure, events, but the only thing consuming resources is the LobbyState)
	this->m_screenWidth = m_app->getWindow()->getWindowWidth();
	this->m_screenHeight = m_app->getWindow()->getWindowHeight();


	NWrapperSingleton::getInstance().checkForPackages();

	if (NWrapperSingleton::getInstance().isHost() && m_settingsChanged && m_timeSinceLastUpdate > 0.2f) {
		auto& stat = m_app->getSettings().gameSettingsStatic;
		auto& dynamic = m_app->getSettings().gameSettingsDynamic;
		m_network->updateGameSettings(m_app->getSettings().serialize(stat, dynamic));
		m_settingsChanged = false;
		m_timeSinceLastUpdate = 0.0f;
	}
	m_timeSinceLastUpdate += dt;

	return false;
}

bool LobbyState::render(float dt, float alpha) {
	m_app->getAPI()->clear({ 0.1f, 0.2f, 0.3f, 1.0f });
	ECS::Instance()->getSystem<BeginEndFrameSystem>()->renderNothing();
	return false;
}

bool LobbyState::renderImgui(float dt) {

	//ImGui::ShowDemoWindow();
	static std::string font = "Beb30";
	//ImGui::PushFont(m_imGuiHandler->getFont(font));
	//
	//if (ImGui::Begin("IMGUISETTINGS")) {
	//	//ImGui::BeginCombo("##FONTS", &font.front());
	//	for (auto const& [key, val] : m_imGuiHandler->getFontMap()) {
	//		ImGui::PushFont(val);
	//
	//		if (ImGui::Selectable(key.c_str(), font == key)) {
	//			font = key;
	//		}
	//		ImGui::PopFont();
	//	}
	//	//ImGui::EndCombo();
	//}
	//ImGui::End();
	//
	//ImGui::PopFont();

	if (m_usePercentage) {
		m_menuWidth = m_percentage * m_app->getWindow()->getWindowWidth();
	}

	m_size.x = m_menuWidth;
	m_size.y = m_app->getWindow()->getWindowHeight() - m_outerPadding * 2;
	m_pos.x = m_app->getWindow()->getWindowWidth()*0.66f - m_outerPadding - ((m_size.x < m_minSize.x) ? m_minSize.x : m_size.x);
	m_pos.y = m_outerPadding;


	ImGui::PushFont(m_imGuiHandler->getFont(font));
	// ------- menu ----------------
	renderMenu();

	// ------- player LIST ------- 
	renderPlayerList();

	// ------- CHAT LOG ------- 
	renderChat();

	// -------- SETTINGS ----------
	if (m_windowToRender == 1) {
		renderGameSettings();
	}
	
	if (m_windowToRender == 2) {
		ImGui::SetNextWindowPos(m_pos);
		ImGui::SetNextWindowSize(m_size);
		ImGui::SetNextWindowSizeConstraints(m_minSize, m_maxSize);

		//ImGui::SetNextWindowSize(ImVec2(500, 500));
		if (ImGui::Begin("##OptionsMenu", nullptr, m_backgroundOnlyflags)) {

			ImGui::PushFont(m_imGuiHandler->getFont("Beb40"));
			SailImGui::HeaderText("Options");
			ImGui::PopFont();
			ImGui::Separator();
			m_optionsWindow.renderWindow();
		}
		ImGui::End();
	}

	ImGui::PopFont();

	return false;
}

bool LobbyState::onEvent(const Event& event) {
	State::onEvent(event);

	switch (event.type) {

	case Event::Type::TEXTINPUT:			onMyTextInput((const TextInputEvent&)event); break;
	case Event::Type::NETWORK_CHAT:			onRecievedText((const NetworkChatEvent&)event); break;
	case Event::Type::NETWORK_JOINED:		onPlayerJoined((const NetworkJoinedEvent&)event); break;
	case Event::Type::NETWORK_DISCONNECT:	onPlayerDisconnected((const NetworkDisconnectEvent&)event); break;
	case Event::Type::NETWORK_PLAYER_REQUESTED_TEAM_CHANGE:	onPlayerTeamRequest((const NetworkPlayerRequestedTeamChange&)event); break;
	case Event::Type::NETWORK_PLAYER_CHANGED_TEAM:	onPlayerTeamChanged((const NetworkPlayerChangedTeam&)event); break;

	default:
		break;
	}

	return true;
}

void LobbyState::resetCurrentMessage() {
	m_currentmessageIndex = 0;
	for (size_t i = 0; i < m_messageSizeLimit; i++) {
		m_currentmessage[i] = '\0';
	}
}

std::string LobbyState::fetchMessage()
{
	std::string message = std::string(m_currentmessage);

	// Reset currentMessage
	m_currentmessageIndex = 0;
	for (size_t i = 0; i < m_messageSizeLimit; i++) {
		m_currentmessage[i] = '\0';
	}

	return message;
}

void LobbyState::addMessageToChat(const Message& message) {
	// Replace '0: Blah blah message' --> 'Daniel: Blah blah message'
	// Add sender to the text

	std::string msg;
	if (message.senderID != 255) {
		Player* playa = NWrapperSingleton::getInstance().getPlayer(message.senderID);
		msg = playa->name + ": ";
	}
	msg += message.content;
	// Add message to chatlog
	m_messages.push_back(msg);

	// New messages replace old
	if (m_messages.size() > m_messageLimit) {
		m_messages.pop_front();
	}
}

bool LobbyState::onRecievedText(const NetworkChatEvent& event) {
	addMessageToChat(event.chatMessage);
	return true;
}

bool LobbyState::onPlayerJoined(const NetworkJoinedEvent& event) {
	
	if (NWrapperSingleton::getInstance().isHost()) {

		NWrapperSingleton::getInstance().getNetworkWrapper()->setTeamOfPlayer(0, event.player.id);

		NWrapperSingleton::getInstance().getNetworkWrapper()->setClientState(States::JoinLobby, event.player.id);
	}
	
	Message message;
	message.content = event.player.name + " joined the game!";
	message.senderID = 255;
	addMessageToChat(message);
	return true;
}

bool LobbyState::onPlayerDisconnected(const NetworkDisconnectEvent& event) {
	Message message;
	message.content = event.player.name;
	message.content += (event.reason == PlayerLeftReason::KICKED) ? " was kicked!" : " left the game!";

	message.senderID = 255;
	addMessageToChat(message);
	return true;
}

bool LobbyState::onPlayerTeamRequest(const NetworkPlayerRequestedTeamChange& event) {
	
	if (NWrapperSingleton::getInstance().isHost()) {
		NWrapperSingleton::getInstance().getNetworkWrapper()->setTeamOfPlayer(event.team, event.playerID);
	}

	return true;
}

bool LobbyState::onPlayerTeamChanged(const NetworkPlayerChangedTeam& event) {	
	///PRINT MESSAGE
	std::unordered_map<std::string, SettingStorage::Setting>& gamemodeSettings = m_settings->gameSettingsStatic["gamemode"];
	SettingStorage::Setting& selectedGameTeams = m_settings->gameSettingsStatic["Teams"][gamemodeSettings["types"].getSelected().name];

	Player* p = NWrapperSingleton::getInstance().getPlayer(event.playerID);

	int currentlySelected = 0;
	for (auto t : selectedGameTeams.options) {
		if ((int)(t.value) == (int)(p->team)) {
			break;
		}
		currentlySelected++;
	}

	Message message;
	message.senderID = 255;
	message.content = NWrapperSingleton::getInstance().getMyPlayerID() == event.playerID ? "You" : NWrapperSingleton::getInstance().getPlayer(event.playerID)->name;
	message.content += " changed team to " + selectedGameTeams.options[currentlySelected].name;
	addMessageToChat(message);

	return true;
}

void LobbyState::renderPlayerList() {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	flags |= ImGuiWindowFlags_NoResize;
	flags |= ImGuiWindowFlags_NoMove;
	flags |= ImGuiWindowFlags_NoNav;
	flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	flags |= ImGuiWindowFlags_NoTitleBar;
	flags |= ImGuiWindowFlags_NoSavedSettings;

	ImGui::SetNextWindowPos(ImVec2(
		m_screenWidth - m_outerPadding - 430,
		m_outerPadding
	));
	static ImVec2 windowSize(400, 500);
	ImGui::SetNextWindowSize(windowSize);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
	
	static float x[3] = {
		ImGui::GetWindowContentRegionWidth() * 0.5f,
		ImGui::GetWindowContentRegionWidth() * 0.84f,
		0
	};

	if (ImGui::Begin("players in lobby:", NULL, flags)) {

		//ImGui::DragFloat3("##ASDASD", &x[0], 0.1f);
		ImGui::Separator();
		ImGui::Text("Player"); 
		ImGui::SameLine(x[0]);

		ImGui::Text("Team"); 
		ImGui::SameLine(x[1]);

		ImGui::Text("Ready"); 
		ImGui::Separator();

		//(windowSize.x) * 0.42f
		std::unordered_map<std::string, SettingStorage::Setting>& gamemodeSettings = m_settings->gameSettingsStatic["gamemode"];

		SettingStorage::Setting& selectedGameTeams = m_settings->gameSettingsStatic["Teams"][gamemodeSettings["types"].getSelected().name];
		unsigned char myID = NWrapperSingleton::getInstance().getMyPlayerID();
		for (auto currentplayer : NWrapperSingleton::getInstance().getPlayers()) {			
			ImGui::BeginGroup();
			ImGui::Text(std::string(currentplayer.name + std::string((currentplayer.id == myID) ? "*" : "")).c_str());
			ImGui::SameLine(x[0]);
			// TEAM
			if (currentplayer.id == myID || myID == HOST_ID) {
				char selectedTeam = NWrapperSingleton::getInstance().getPlayer(currentplayer.id)->team;
				std::string unique = "##LABEL" + std::to_string(currentplayer.id);

				int currentlySelected = 0;
				for (auto t : selectedGameTeams.options) {
					if ((int)(t.value) == (int)(currentplayer.team)) {
						break;
					}
					currentlySelected++;
				}

				currentlySelected = std::clamp(currentlySelected, 0, (int)(selectedGameTeams.options.size()) - 1);
				ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.3f);
				if (ImGui::BeginCombo(unique.c_str(), selectedGameTeams.options[currentlySelected].name.c_str())) {
					for (auto const& key : selectedGameTeams.options) {
						std::string name = key.name + unique;

						if (ImGui::Selectable(name.c_str(), selectedTeam == (char)key.value)) {
							if (currentplayer.id == myID) {
								NWrapperSingleton::getInstance().getNetworkWrapper()->requestTeam(key.value);
							} else {
								NWrapperSingleton::getInstance().getNetworkWrapper()->setTeamOfPlayer(key.value, currentplayer.id);						
							}
						}
					}
					ImGui::EndCombo();
				}
			} 
			else {
				std::string s = selectedGameTeams.options.back().name;
				
				for (auto t : selectedGameTeams.options) {
					if ((int)(t.value) == (int)(currentplayer.team)) {
						s = t.name;
						break;
					}
				}

				ImGui::Text(s.c_str());
			}
			ImGui::SameLine(x[1]);

			// READY 
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			bool asd = currentplayer.lastStateStatus.state == States::Lobby && currentplayer.lastStateStatus.status > 0;
			ImGui::Checkbox(std::string("##Player" + std::to_string(currentplayer.id)).c_str(), &asd); 
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
			ImGui::EndGroup();
			if (ImGui::BeginPopupContextItem(std::string("item context menu##" + std::to_string(currentplayer.id)).c_str())) {
				if (NWrapperSingleton::getInstance().isHost()) {
					if (ImGui::Button("KICK")) {
						NWrapperSingleton::getInstance().getNetworkWrapper()->kickPlayer(currentplayer.id);
					}
				}
				ImGui::EndPopup();
			}
			
			//ImGui::OpenPopupOnItemClick(std::string("item context menu##" + std::to_string(currentplayer.id)).c_str(), 1);

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("First group hovered");
			ImGui::Separator(); 
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();

}

void LobbyState::renderGameSettings() {
	ImGuiWindowFlags settingsFlags = ImGuiWindowFlags_NoCollapse;
	settingsFlags |= ImGuiWindowFlags_NoResize;
	settingsFlags |= ImGuiWindowFlags_NoMove;
	settingsFlags |= ImGuiWindowFlags_NoNav;
	settingsFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	settingsFlags |= ImGuiWindowFlags_NoTitleBar;
	settingsFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	settingsFlags |= ImGuiWindowFlags_NoSavedSettings;


	// Uncomment when we actually have game settings
	ImGui::SetNextWindowPos(m_pos);
	ImGui::SetNextWindowSize(m_size);
	ImGui::SetNextWindowSizeConstraints(m_minSize, m_maxSize);
	if (ImGui::Begin("##LOBBYSETTINGS", nullptr, settingsFlags)) {
		ImGui::PushFont(m_imGuiHandler->getFont("Beb40"));
		SailImGui::HeaderText("Lobby Settings");
		ImGui::PopFont();
		ImGui::Separator();


		m_optionsWindow.renderGameOptions();
	}
	ImGui::End();
}

void LobbyState::renderChat() {
	ImGuiWindowFlags chatFlags = ImGuiWindowFlags_NoCollapse;
	chatFlags |= ImGuiWindowFlags_NoResize;
	chatFlags |= ImGuiWindowFlags_NoMove;
	chatFlags |= ImGuiWindowFlags_NoNav;
	chatFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	chatFlags |= ImGuiWindowFlags_NoTitleBar;
	chatFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	chatFlags |= ImGuiWindowFlags_NoSavedSettings;
	chatFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
	chatFlags |= ImGuiWindowFlags_NoInputs;

	// ------- message BOX ------- 
	ImGui::SetNextWindowPos(ImVec2(
		m_outerPadding,
		m_screenHeight - (m_outerPadding + m_textHeight)
	));
	ImGui::Begin(
		"Write Here",
		NULL,
		chatFlags
	);

	if (m_firstFrame) {
		m_firstFrame = false;
		m_chatFocus = false;
	}
	ImGui::Text("Enter message:");
	if (ImGui::InputText("", m_currentmessage, m_messageSizeLimit, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_EnterReturnsTrue)) {
		m_chatFocus = false;
	}
	ImGui::End();

	// ------- CHAT LOG ------- 
	ImGui::SetNextWindowSize(ImVec2(
		400,
		300
	));
	ImGui::SetNextWindowPos(ImVec2(
		m_outerPadding,
		m_screenHeight - (300 + m_outerPadding)
	));

	// Render message history
	ImGui::Begin("Chat Log", NULL, chatFlags);
	ImGui::SameLine();
	ImGui::BeginChild("messages");
	for (auto currentmessage : m_messages) {
		ImGui::Text(
			currentmessage.c_str()
		);
	}
	ImGui::EndChild();
	ImGui::End();
}

void LobbyState::renderMenu() {
	static ImVec2 pos(m_outerPadding, m_outerPadding );
	static ImVec2 size(350, 400);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	ImGui::PushFont(m_imGuiHandler->getFont("Beb60"));

	if (ImGui::Begin("##LOBBYMENU", nullptr, m_standaloneButtonflags)) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.3f, 0.3f, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1));
		if (SailImGui::TextButton("Leave")) {

			// Reset the network
			NWrapperSingleton::getInstance().resetNetwork();
			NWrapperSingleton::getInstance().resetWrapper();

			// Schedule new state change
			this->requestStackPop();
			this->requestStackPush(States::MainMenu);
		}
		ImGui::PopStyleColor(2);
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		if (m_windowToRender == 1) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		if (SailImGui::TextButton((m_windowToRender == 1) ? ">Game Options" : "Game Options")) {
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
		if (SailImGui::TextButton((m_windowToRender == 2) ? ">Options" : "Options")) {
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


		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1));

		
		if (NWrapperSingleton::getInstance().isHost()) {
			bool allReady = true;
			for (auto p : NWrapperSingleton::getInstance().getPlayers()) {
				if (p.lastStateStatus.state != States::Lobby || p.lastStateStatus.status < 1) {
					allReady = false;
				}
			}

			if (SailImGui::TextButton((allReady) ? "Start" : "Force start")) {
				// Queue a removal of LobbyState, then a push of gamestate
				NWrapperSingleton::getInstance().stopUDP();
				//m_app->getStateStorage().setLobbyToGameData(LobbyToGameData(*m_settingBotCount, m_teamSelection));
				
				auto& stat = m_app->getSettings().gameSettingsStatic;
				auto& dynamic = m_app->getSettings().gameSettingsDynamic;

				//TODO: ONLY DO THIS IF GAMEMODE IS FFA
				int teamID = 0;
				for (auto p : NWrapperSingleton::getInstance().getPlayers()) {
					if (p.team != -1) {
						NWrapperSingleton::getInstance().getNetworkWrapper()->setTeamOfPlayer(teamID % 12, p.id, false);
						teamID++;
					}
				}

				m_network->updateGameSettings(m_app->getSettings().serialize(stat, dynamic));
				m_network->setClientState(States::Game);

				this->requestStackClear();
				this->requestStackPush(States::Game);
			}
		}
		else {
			if (m_ready) {
				if (SailImGui::TextButton("unReady")) {
					NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::Lobby, 0);
					m_ready = false;
				}
			}
			else {
				if (SailImGui::TextButton("Ready")) {
					NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::Lobby, 1);
					m_ready = true;
				}
			}

		}
		ImGui::PopStyleColor(2);


	}
	ImGui::End();
	ImGui::PopFont();
}

