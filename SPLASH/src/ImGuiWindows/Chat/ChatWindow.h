#pragma once

#include "Sail/utils/SailImGui/SailImGuiWindow.h"
#include "Sail/events/Events.h"
#include "Sail/events/Event.h"
#include "Sail/events/EventReceiver.h"
#include <map>

class Application;
class ImGuiHandler;
class NWrapperSingleton;
class SettingStorage;

struct ChatSent;
struct NetworkChatEvent;
struct NetworkJoinedEvent;
struct NetworkDisconnectEvent;
struct NetworkPlayerChangedTeam;
struct WindowResizeEvent;

class ChatWindow : public SailImGuiWindow, public EventReceiver {
public:
	struct ChatMessage {
		int id = 0;
		std::string name;
		std::string message;
	};

	ChatWindow(bool showWindow = true);
	~ChatWindow();
	virtual void renderWindow() override;
	void renderChat(float dt);

	void addSystemMessage(const std::string& message);
	void addMessage(const int id, const std::string& name, const std::string message);
	void clearHistory();



	void setFadeTime(const float& time);
	void setFadeThreshold(const float& time);
	void resetMessageTime();
	void setRetainFocus(const bool retain);
	void removeFocus();
	void setBackgroundOpacity(const float& opacity);



protected:

	virtual bool onEvent(const Event& event) override;
	bool onMyTextInput(const ChatSent& event);
	bool onRecievedText(const NetworkChatEvent& event);
	bool onPlayerJoined(const NetworkJoinedEvent& event);
	bool onPlayerDisconnected(const NetworkDisconnectEvent& event);
	bool onPlayerTeamChanged(const NetworkPlayerChangedTeam& event);
	bool onWindowResize(const WindowResizeEvent& event);


private:
	Application* m_app;
	SettingStorage* m_settings;
	ImGuiHandler* m_imguiHandler;
	EventDispatcher* m_eventDispatcher;
	NWrapperSingleton* m_network;

	std::string m_message;
	std::list<ChatMessage> m_messages;
	float m_timeSinceLastMessage;
	float m_fadeTime;
	float m_fadeThreshold;
	bool m_scrollToBottom;
	unsigned int m_messageLimit;
	unsigned int m_messageSizeLimit;
	bool m_retainFocus;

	float m_backgroundOpacityMul;
	bool m_removeFocus;
};