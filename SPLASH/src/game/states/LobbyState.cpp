#include "LobbyState.h"


//#include "../imgui-sfml-master/imgui-SFML.h"
#include "../libraries/imgui/imgui.h"
#include "../Sail/src/API/DX12/imgui/DX12ImGuiHandler.h"
#include "../SPLASH/src/game/events/TextInputEvent.h"
#include "../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "Network/NWrapperSingleton.h"	// New network
#include "Network/NWrapper.h"			// 
#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/Audio/AudioSystem.h"
#include "Sail/entities/components/AudioComponent.h"

#include <string>
#include <list>

LobbyState::LobbyState(StateStack& stack)
	: State(stack)
{
	// ImGui is already initiated and set up, thanks alex!
	m_app = Application::getInstance();
	m_input = Input::GetInstance();
	m_network = NWrapperSingleton::getInstance().getNetworkWrapper();
	m_textHeight = 52;
	m_outerPadding = 15;

	m_messageLimit = 14;		// Should be based on size of chatbox instead of fixed
	m_messageCount = 0;
	m_settingBotCount = new int;
	*m_settingBotCount = 0;

	m_messageSizeLimit = 50;
	m_currentmessageIndex = 0;
	m_currentmessage = SAIL_NEW char[m_messageSizeLimit] { 0 };

	// TO DO: Streaming sounds in menu doesn't work because ESC is NOT UPDATED HERE
	//m_lobbyAudio = ECS::Instance()->createEntity("LobbyAudio").get();
	//m_lobbyAudio->addComponent<AudioComponent>();
	//m_lobbyAudio->getComponent<AudioComponent>()->streamSoundRequest_HELPERFUNC("../Audio/LobbyMusic.xwb", true, true);
}


LobbyState::~LobbyState() {
	delete[] m_currentmessage;
	delete m_settingBotCount;

}

bool LobbyState::processInput(float dt) {
	if (m_input->IsMouseButtonPressed(0)) {
		m_chatFocus = false;
	}

	return false;
}

bool LobbyState::inputToChatLog(MSG& msg) {
	int sendMessageKeyCode = KeyBinds::sendMessage;
	if (m_currentmessageIndex < m_messageSizeLimit && msg.wParam != sendMessageKeyCode) {
		// Add whichever button that was inputted to the current message
		// --- OBS : doesn't account for capslock, etc.
		m_currentmessage[m_currentmessageIndex++] = (char)msg.wParam;
	}
	if (msg.wParam == sendMessageKeyCode && m_chatFocus == false) {
		return true;
	}
	return false;
}

bool LobbyState::update(float dt, float alpha) {
	// Update screen dimensions & ImGui related
	// (Sure, events, but the only thing consuming resources is the LobbyState)
	this->m_screenWidth = m_app->getWindow()->getWindowWidth();
	this->m_screenHeight = m_app->getWindow()->getWindowHeight();

	m_network->checkForPackages();

	return false;
}

bool LobbyState::render(float dt, float alpha) {
	m_app->getAPI()->clear({ 0.1f, 0.2f, 0.3f, 1.0f });
	ECS::Instance()->getSystem<BeginEndFrameSystem>()->renderNothing();
	return false;
}

bool LobbyState::renderImgui(float dt) {
	// ------- player LIST ------- 
	renderPlayerList();

	// ------- CHAT LOG ------- 
	renderChat();

	// -------- SETTINGS ----------
	renderSettings();

	// ------- START BUTTON ------- 
	renderStartButton();

	// ------- Return BUTTON ------- 
	renderQuitButton();

	return false;
}

void LobbyState::addTextToChat(Message* message) {
	this->addMessageToChat(*message);
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

void LobbyState::addMessageToChat(Message& message) {
	// Replace '0: Blah blah message' --> 'Daniel: Blah blah message'
	// Add sender to the text
	unsigned char id = stoi(message.sender);
	Player* playa = NWrapperSingleton::getInstance().getPlayer(id);
	std::string msg = playa->name + ": ";
	message.content.insert(0, msg);

	// Add message to chatlog
	m_messages.push_back(message);

	// New messages replace old
	if (m_messages.size() > m_messageLimit) {
		m_messages.pop_front();
	}
}

void LobbyState::renderPlayerList() {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	flags |= ImGuiWindowFlags_NoResize;
	flags |= ImGuiWindowFlags_NoMove;
	flags |= ImGuiWindowFlags_NoNav;
	flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	flags |= ImGuiWindowFlags_NoTitleBar;
	flags |= ImGuiWindowFlags_AlwaysAutoResize;
	flags |= ImGuiWindowFlags_NoSavedSettings;

	ImGui::SetNextWindowPos(ImVec2(
		m_outerPadding,
		m_outerPadding
	));
	ImGui::Begin("players in lobby:", NULL, flags);

	unsigned char myID = NWrapperSingleton::getInstance().getMyPlayerID();
	for (auto currentplayer : NWrapperSingleton::getInstance().getPlayers()) {
		std::string temp;
		temp += " - ";
		temp += currentplayer.name.c_str();

		if (currentplayer.id == myID) {
			temp += " (You)";
		}
		
		ImGui::Text(temp.c_str());
	}
	ImGui::End();
}

void LobbyState::renderStartButton() {
	if (NWrapperSingleton::getInstance().isHost()) {
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
		flags |= ImGuiWindowFlags_NoResize;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoNav;
		flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		flags |= ImGuiWindowFlags_NoSavedSettings;
		ImGui::SetNextWindowPos(ImVec2(
			m_screenWidth - (m_outerPadding + m_screenWidth / 10.0f),
			m_screenHeight - (m_outerPadding + m_screenHeight / 10.0f)
		));
		ImGui::Begin("Start Game");

		if (ImGui::Button("S.P.L.A.S.H")) {
			// Queue a removal of LobbyState, then a push of gamestate
			m_app->getStateStorage().setLobbyToGameData(LobbyToGameData(*m_settingBotCount));
			m_network->sendMsgAllClients("t");
			this->requestStackPop();
			this->requestStackPush(States::Game);
		}
		ImGui::End();
	}
}

void LobbyState::renderQuitButton() {
	ImGui::Begin("Return");
	if (ImGui::Button("Return to Lobby")) {
	
		// Set temp window pos
		ImGui::SetWindowPos({ 424,641 });
		ImGui::SetWindowSize({ 117,63 });

		// Reset the network
		NWrapperSingleton::getInstance().resetNetwork();
		NWrapperSingleton::getInstance().resetWrapper();

		// Schedule new state change
		this->requestStackPop();
		this->requestStackPush(States::MainMenu);
	}
	ImGui::End();
}

void LobbyState::renderSettings() {
	ImGuiWindowFlags settingsFlags = ImGuiWindowFlags_NoCollapse;
	settingsFlags |= ImGuiWindowFlags_NoResize;
	settingsFlags |= ImGuiWindowFlags_NoMove;
	settingsFlags |= ImGuiWindowFlags_NoNav;
	settingsFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	settingsFlags |= ImGuiWindowFlags_NoTitleBar;
	settingsFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	settingsFlags |= ImGuiWindowFlags_NoSavedSettings;

	ImGui::SetNextWindowPos(ImVec2(
		m_screenWidth - m_outerPadding - 330,
		m_outerPadding
	));
	ImGui::Begin("Settings", NULL, settingsFlags);
	ImGui::InputInt("BotCountInput: ", m_settingBotCount, 1, 1);

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
			currentmessage.content.c_str()
		);
	}
	ImGui::EndChild();
	ImGui::End();
}
