#pragma once

#include "Sail.h"
#include <string>
#include <list>
#include "Network/NetworkWrapper.h"
using namespace std;

class Input;
class TextInputEvent;
class NetworkJoinedEvent;

struct message {
	string sender;
	string content;
};

struct player {
	unsigned int id;
	string name;

	bool friend operator==(const player& left, const player& right) {
		if (left.id == right.id &&
			left.name == right.name) {
			return true;
		}
		return false;
	}
};

class LobbyState : public State {
public:
	LobbyState(StateStack& stack);
	virtual ~LobbyState();

	// Process input for the state
	bool processInput(float dt);
	// Updates the state
	virtual bool update(float dt);
	// Renders the state
	bool render(float dt);
	// Renders imgui
	bool renderImgui(float dt);
	// Sends events to the state
	virtual bool onEvent(Event& event) = 0;

protected:
	Application* m_app = nullptr;
	Input* m_input = nullptr;
	NetworkWrapper* m_network = nullptr;
	char* m_currentmessage = nullptr;
	string m_myName;
	std::list<message> m_messages;
	std::list<player> m_players;

	// Front-End Functions
	bool inputToChatLog(MSG& msg);
	bool playerJoined(player player);
	bool playerLeft(unsigned int id);
	void addTextToChat(const string* text);
	void resetCurrentMessage();

	void appendMSGToCurrentMessage();
	void sendMessage();
	string fetchMessage();
	void addMessageToChat(const string* text, const player* sender);

private:
	std::unique_ptr<ImGuiHandler> m_imGuiHandler;
	// Front-end functions - Use these!	
	
	void recieveMessage(string text, unsigned int senderID);
	// Back-end functions

	player* getPlayer(unsigned int id);
	// Back-end variables
	unsigned int m_currentmessageIndex;
	unsigned int m_messageSizeLimit;
	unsigned int m_playerCount;
	unsigned int m_playerLimit;
	unsigned int m_messageCount;
	unsigned int m_messageLimit;
	bool m_firstFrame = true;	// Used solely for ImGui
	bool m_chatFocus = true;	// Used solely for ImGui
	unsigned int m_tempID = 0; // used as id counter until id's are gotten through network shit.
	Scene m_scene;

	// Purely for testing
	void addTestData();

	// Render ImGui Stuff --------- WILL BE REPLACED BY OTHER GRAPHICS.
	unsigned int m_outerPadding;
	unsigned int m_screenWidth;
	unsigned int m_screenHeight;
	unsigned int m_textHeight;
	void renderPlayerList();
	void renderStartButton();
	void renderSettings();		// Currently empty
	void renderChat();
};