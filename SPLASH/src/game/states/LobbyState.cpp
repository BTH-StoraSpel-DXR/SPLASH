#include "LobbyState.h"


//#include "../imgui-sfml-master/imgui-SFML.h"
#include "../libraries/imgui/imgui.h"
#include "../Sail/src/API/DX12/imgui/DX12ImGuiHandler.h"
#include "../SPLASH/src/game/events/TextInputEvent.h"
#include "../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "Network/NetworkWrapper.h"
#include "Sail/api/Input.h"
#include "Sail/api/Window.h"

#include <string>
using namespace std;

LobbyState::LobbyState(StateStack& stack)
	: State(stack)
{
	// ImGui is already initiated and set up, thanks alex!
	m_app = Application::getInstance();
	m_input = Input::GetInstance();
	m_network = &NetworkWrapper::getInstance();
	m_textHeight = 52;
	m_outerPadding = 15;

	m_messageLimit = 14;		// Should be based on size of chatbox instead of fixed
	m_playerLimit = 12;
	m_playerCount = 0;
	m_messageCount = 0;

	// Add local player as the first.
	m_myName = "Daniel";
	playerJoined(player{ m_tempID++, m_myName });

	m_messageSizeLimit = 50;
	m_currentmessageIndex = 0;
	m_currentmessage = new char[m_messageSizeLimit] { 0 };

	// -------- test -------- 
	//addTestData();
}

LobbyState::~LobbyState() {
	delete[]m_currentmessage;
}

bool LobbyState::processInput(float dt) {
	// Did user want to enter anything in the chatbox?
	// Did user want to send the message?
	// ---

	if (m_input->IsMouseButtonPressed(0)) {
		m_chatFocus = false;
	}

	// Purely a function for testing
//	this->doTestStuff();

	// Did user want to change some setting?
	// ---

	// Did user want to start the game?
	// ---

	return false;
}

bool LobbyState::inputToChatLog(MSG& msg)
{
	if (m_currentmessageIndex < m_messageSizeLimit && msg.wParam != SAIL_KEY_RETURN) {
		// Add whichever button that was inputted to the current message
		// --- OBS : doesn't account for capslock, etc.
		m_currentmessage[m_currentmessageIndex++] = (char)msg.wParam;
	}
	if (msg.wParam == SAIL_KEY_RETURN && m_chatFocus == false) {
		return true;
	}
	return false;
}

bool LobbyState::update(float dt) {
	// Update screen dimensions & ImGui related
	// (Sure, events, but the only thing consuming resources is the LobbyState)
	this->m_screenWidth = m_app->getWindow()->getWindowWidth();
	this->m_screenHeight = m_app->getWindow()->getWindowHeight();

	// Did we send something?
	// ---
	if (NetworkWrapper::getInstance().isInitialized())
	{
		NetworkWrapper::getInstance().checkForPackages();
	}

	// Did we recieve something?
	// ---

	// Is anything going on in the background?
	// ---

	return false;
}

bool LobbyState::render(float dt) {
	m_app->getAPI()->clear({ 0.1f, 0.2f, 0.3f, 1.0f });
	m_scene.draw();
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

	return false;
}

bool LobbyState::playerJoined(player player) {
	if (m_playerCount < m_playerLimit) {
		m_players.push_back(player);
		m_playerCount++;
		return true;
	}
	return false;
}

bool LobbyState::playerLeft(unsigned int id) {
	// Linear search to get target 'player' struct, then erase that from the list
	player* toBeRemoved = nullptr;
	int pos = 0;
	for (auto playerIt : m_players) {
		if (playerIt.id == id) {
			toBeRemoved = &playerIt;
			m_players.remove(*toBeRemoved);
			return true;
		}
	}

	return false;
}

void LobbyState::addTextToChat(const string* text) {
	this->addMessageToChat(text, &m_players.front());
}

void LobbyState::resetCurrentMessage() {
	m_currentmessageIndex = 0;
	for (size_t i = 0; i < m_messageSizeLimit; i++) {
		m_currentmessage[i] = '\0';
	}
}

void LobbyState::appendMSGToCurrentMessage()
{
}

void LobbyState::sendMessage() {
	if (m_currentmessageIndex != 0) { // If the message isn't empty
		//this->addmessageToChat(text, &m_players.front());

		// Reset currentMessage
		m_currentmessageIndex = 0;
		for (size_t i = 0; i < m_messageSizeLimit; i++) {
			m_currentmessage[i] = '\0';
		}
	}
}

string LobbyState::fetchMessage()
{
	string message = string(m_currentmessage);

	// Reset currentMessage
	m_currentmessageIndex = 0;
	for (size_t i = 0; i < m_messageSizeLimit; i++) {
		m_currentmessage[i] = '\0';
	}

	return message;
}

void LobbyState::recieveMessage(string text, unsigned int senderID) {
	addMessageToChat(&text, getPlayer(senderID));
}

void LobbyState::addMessageToChat(const string* text, const player* sender) {
	// Add message to chatlog
	m_messages.push_back(message{
		sender->name,
		*text
		});

	// New messages replace old
	if (m_messages.size() > m_messageLimit) {
		m_messages.pop_front();
	}
}

player* LobbyState::getPlayer(unsigned int id) {
	for (auto playerIt : m_players) {
		if (playerIt.id == id) {
			return &playerIt;
		}
	}
	return nullptr;
}

void LobbyState::addTestData()
{
	// Set up players
//	playerJoined("Ollie", m_tempID++);
//	playerJoined("David", m_tempID++);
	//playerJoined("Press 0 to switch between enter msg and going to gamestate with mouse (MouseClick by default)");
	//playerJoined("The cause of this is an IMGUI-bug where keyboard focus prevents buttons from working");
	//playerJoined("This tape is since we'll switch from imgui later anyway, so why patch shit that's only an imgui bug");
	//playerJoined("If u want to look at it: https://github.com/ocornut/imgui/issues/455#event-1428367449");

	message msg;
	msg.sender = "Daniel";
	msg.content = "msg1";
	m_messages.push_back(msg);

	msg.sender = "Ollie";
	msg.content = "message Two";
	m_messages.push_back(msg);

	msg.sender = "David";
	msg.content = "msg 3 mr.boss";
	m_messages.push_back(msg);
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

	for (auto currentplayer : m_players) {
		ImGui::Text(
			std::string("- ").append(currentplayer.name.c_str()).c_str()
		);
	}
	ImGui::End();
}

void LobbyState::renderStartButton() {
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
	ImGui::Begin("Press 0 once");

	// SetKeyBoardFocusHere on the chatbox prevents the button from working,
	// so if we click with the mouse, temporarily set focus to the button.

	if (ImGui::Button("S.P.L.A.S.H")) {
		// Queue a removal of LobbyState, then a push of gamestate
		this->requestStackPop();
		this->requestStackPush(States::Game);
	}
	ImGui::End();
}

void LobbyState::renderSettings() {

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
		ImGui::SetKeyboardFocusHere(-1);
		m_firstFrame = false;
		m_chatFocus = true;
	}
	ImGui::Text("Enter message:");
	if (ImGui::InputText("", m_currentmessage, m_messageSizeLimit, ImGuiInputTextFlags_EnterReturnsTrue)) {
		//this->sendmessage(&(string)m_currentmessage);
		ImGui::SetKeyboardFocusHere(-1);
		m_chatFocus = true;
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