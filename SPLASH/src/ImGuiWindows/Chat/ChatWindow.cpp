#include "pch.h"
#include "ChatWindow.h"
#include "imgui.h"
#include "Sail/Application.h"
#include "Network/NWrapperSingleton.h"

#include "../SPLASH/src/game/events/TextInputEvent.h"
#include "../SPLASH/src/game/events/NetworkChatEvent.h"
#include "../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "../SPLASH/src/game/events/NetworkDisconnectEvent.h"
#include "Sail/events/types/NetworkPlayerChangedTeam.h"
#include "Sail/events/types/WindowResizeEvent.h"

ChatWindow::ChatWindow(bool showWindow) :
	m_message(""),
	m_timeSinceLastMessage(0.0f),
	m_fadeTime(-1.0f),
	m_fadeThreshold(-1.0f),
	m_backgroundOpacityMul(0.4f),
	m_scrollToBottom(false),
	m_messageLimit(30),
	m_messageSizeLimit(150),
	m_retainFocus(true),
	m_removeFocus(false)
{
	m_app = Application::getInstance();
	m_settings = &m_app->getSettings();
	m_imguiHandler = m_app->getImGuiHandler();
	m_eventDispatcher = &EventDispatcher::Instance();
	m_network = &NWrapperSingleton::getInstance();


	m_eventDispatcher->subscribe(Event::Type::CHATSENT, this);
	m_eventDispatcher->subscribe(Event::Type::NETWORK_CHAT, this);
	m_eventDispatcher->subscribe(Event::Type::NETWORK_JOINED, this);
	m_eventDispatcher->subscribe(Event::Type::NETWORK_DISCONNECT, this);
	m_eventDispatcher->subscribe(Event::Type::NETWORK_PLAYER_CHANGED_TEAM, this);
	m_eventDispatcher->subscribe(Event::Type::WINDOW_RESIZE, this);






}

ChatWindow::~ChatWindow() {
	m_eventDispatcher->unsubscribe(Event::Type::CHATSENT, this);
	m_eventDispatcher->unsubscribe(Event::Type::NETWORK_CHAT, this);
	m_eventDispatcher->unsubscribe(Event::Type::NETWORK_JOINED, this);
	m_eventDispatcher->unsubscribe(Event::Type::NETWORK_DISCONNECT, this);
	m_eventDispatcher->unsubscribe(Event::Type::NETWORK_PLAYER_CHANGED_TEAM, this);
	m_eventDispatcher->unsubscribe(Event::Type::WINDOW_RESIZE, this);
}

bool ChatWindow::onEvent(const Event& event) {


	switch (event.type) {
				
	case Event::Type::CHATSENT:						onMyTextInput((const ChatSent&)event); break;
	case Event::Type::NETWORK_CHAT:					onRecievedText((const NetworkChatEvent&)event); break;
	case Event::Type::NETWORK_JOINED:				onPlayerJoined((const NetworkJoinedEvent&)event); break;
	case Event::Type::NETWORK_DISCONNECT:			onPlayerDisconnected((const NetworkDisconnectEvent&)event); break;
	case Event::Type::NETWORK_PLAYER_CHANGED_TEAM:	onPlayerTeamChanged((const NetworkPlayerChangedTeam&)event); break;
	case Event::Type::WINDOW_RESIZE:				onWindowResize((const WindowResizeEvent&)event); break;
	

	default:
		break;
	}

	return true;
}



void ChatWindow::renderWindow() {




}

void ChatWindow::renderChat(float dt) {
	m_timeSinceLastMessage += dt;

	ImGuiWindowFlags chatFlags = ImGuiWindowFlags_NoCollapse;
	chatFlags |= ImGuiWindowFlags_NoResize;
	chatFlags |= ImGuiWindowFlags_NoMove;
	chatFlags |= ImGuiWindowFlags_NoNav;
	chatFlags |= ImGuiWindowFlags_NoTitleBar;
	chatFlags |= ImGuiWindowFlags_NoSavedSettings;


	ImGui::SetNextWindowPos(m_position);
	ImGui::SetNextWindowSize(m_size);
	//ImGui::SetNextWindowSizeConstraints(size); Keep
	static bool focus = false;
	static bool pop = false;
	pop = false;
	if (focus) {
		m_timeSinceLastMessage = 0.0f;
	}
	static float alpha = 1.0f;
	alpha = 1.0f;
	if (m_timeSinceLastMessage > m_fadeThreshold && m_fadeThreshold >= 0.0f) {
		pop = true;
		alpha = 1.0f - ((m_timeSinceLastMessage - m_fadeThreshold) / m_fadeTime);
		alpha = alpha < 0.01f ? 0.01f : alpha;
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

	}

	ImVec4 borderCol(ImGui::GetStyleColorVec4(ImGuiCol_Border));
	borderCol.w*= m_backgroundOpacityMul;
	ImGui::PushStyleColor(ImGuiCol_Border, borderCol);

	ImVec4 bgCol(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	bgCol.w*= m_backgroundOpacityMul;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, bgCol);




	if (ImGui::Begin("##CHATWINDOW", nullptr, chatFlags)) {
		ImGui::PushFont(m_imguiHandler->getFont("Beb20"));
#ifdef DEVELOPMENT
		ImGui::Checkbox("Focus", &m_retainFocus); 
		ImGui::SameLine();
		ImGui::SetNextItemWidth(50);
		ImGui::SliderFloat("Threshold", &m_fadeThreshold, -1.0f, 15.0f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(50);
		ImGui::SliderFloat("time", &m_fadeTime, -1.0f, 15.0f);
		ImGui::SameLine();
		if (ImGui::Button("res")) {
			m_timeSinceLastMessage = 0.0f;
		}
#endif
		if (ImGui::BeginChild("##CHATTEXT", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
			if (alpha <= 0.02f) {
				ImVec4 txtCol(ImGui::GetStyleColorVec4(ImGuiCol_Text));
				txtCol.w = 0.0f;
				ImGui::PushStyleColor(ImGuiCol_Text, txtCol);
			}
			for (auto currentMessage : m_messages) {
				//System or player which has left
				if (currentMessage.id == 255 || currentMessage.id == 254) {
					ImGui::Text(currentMessage.message.c_str());
				}
				//Player
				else {
					if (auto * player = m_network->getPlayer(currentMessage.id)) {
						int team = player->team;
						glm::vec3 temp = m_settings->getColor(m_settings->teamColorIndex(team));
						ImVec4 col(
							temp.r,
							temp.g,
							temp.b,
							1
						);
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
						ImGui::PushStyleColor(ImGuiCol_Text, col);
						ImGui::Text(player->name.c_str());
						ImGui::PopStyleColor();
						ImGui::SameLine();
						ImGui::TextWrapped(std::string(": " + currentMessage.message).c_str());
						ImGui::PopStyleVar();
					}


				}


			}
			if (m_scrollToBottom) {
				ImGui::SetScrollHereY(1.0f);
				m_scrollToBottom = false;
			}
			if (alpha <= 0.02f) {
				ImGui::PopStyleColor();
			}
		}
		ImGui::EndChild();
		ImGui::PopFont();
		static bool sentThisFrame = false;
		static bool justSent = false;
		static bool releasedEnter = true;
		justSent = false;
		static char buf[152] = "";
		strncpy_s(buf, m_message.c_str(), m_message.size());

		if (ImGui::IsKeyReleased(SAIL_KEY_RETURN)) {
			releasedEnter = true;
		}

		// TEXTINPUT
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.9f);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha + 0.3f);

		if (ImGui::InputTextWithHint("##ChatInput", (focus ? "" : "Press enter to chat"), buf, m_messageSizeLimit, ImGuiInputTextFlags_EnterReturnsTrue)) {
			m_message = buf;
			if (m_message != "" && releasedEnter) {
				//Send Message
				m_eventDispatcher->emit(ChatSent(m_message));
				m_message = "";
			}
			releasedEnter = false;
			justSent = true;
	
		}
		else {
			m_message = buf;
		}
		
		ImGui::PopStyleVar();
		focus = ImGui::IsItemActive() || justSent;
		// SENDBUTTON
		ImGui::SameLine();
		if (ImGui::Button(" >  ")) {
			if (m_message != "") {
				m_eventDispatcher->emit(ChatSent(m_message));
				m_message = "";
				justSent = true;
				//sentLastFrame = true;
			}
		}
		if (m_removeFocus) {
			m_removeFocus = false;
			ImGui::SetWindowFocus();
		}
		if (ImGui::IsKeyPressed(SAIL_KEY_RETURN, false)) {
			
			if (m_retainFocus || !justSent) {
				focus = true;
				ImGui::SetKeyboardFocusHere(-1);
			}
		}

	}
	if (pop) {
		ImGui::PopStyleVar();
	}
	ImGui::PopStyleColor(2);

	


	ImGui::End();
}

void ChatWindow::addSystemMessage(const std::string& message) {
	
}

void ChatWindow::addMessage(const int id, const std::string& name, const std::string message) {
	m_timeSinceLastMessage = 0.0f;
	m_messages.push_back({ id, name, message });
	if (m_messages.size() > m_messageLimit) {
		m_messages.pop_front();
	}

	m_scrollToBottom = true;
}

void ChatWindow::clearHistory() {
	m_messages.clear();
}

void ChatWindow::setFadeTime(const float& time) {
	m_fadeTime = time;
}

void ChatWindow::setFadeThreshold(const float& time) {
	m_fadeThreshold = time;
}

void ChatWindow::resetMessageTime() {
	m_timeSinceLastMessage = 0.0f;
}

void ChatWindow::setRetainFocus(const bool retain) {
	m_retainFocus = retain;
}

void ChatWindow::removeFocus() {
	m_removeFocus = true;
}

void ChatWindow::setBackgroundOpacity(const float& opacity) {
	m_backgroundOpacityMul = opacity;
}


bool ChatWindow::onMyTextInput(const ChatSent& event) {

	if (m_network->isHost()) {
		addMessage(
			m_network->getMyPlayer().id,
			m_network->getMyPlayer().name,
			event.msg
		);
	}
	m_network->getNetworkWrapper()->sendChatMsg(event.msg);


	return false;
}

bool ChatWindow::onRecievedText(const NetworkChatEvent& event) {
	addMessage(event.chatMessage.senderID, m_network->getPlayer(event.chatMessage.senderID)->name, event.chatMessage.content);
	return true;
}


bool ChatWindow::onPlayerTeamChanged(const NetworkPlayerChangedTeam& event) {
	auto* settings = &m_app->getSettings();
	std::unordered_map<std::string, SettingStorage::Setting>& gamemodeSettings = settings->gameSettingsStatic["gamemode"];
	SettingStorage::Setting& selectedGameTeams = settings->gameSettingsStatic["Teams"][gamemodeSettings["types"].getSelected().name];

	Player* p = m_network->getPlayer(event.playerID);

	int currentlySelected = 0;
	for (auto t : selectedGameTeams.options) {
		if ((int)(t.value) == (int)(p->team)) {
			break;
		}
		currentlySelected++;
	}
	
	std::string content = m_network->getMyPlayerID() == event.playerID ? "You" : m_network->getPlayer(event.playerID)->name;
	content += " changed team to " + selectedGameTeams.options[currentlySelected > 0].name;
	addMessage(255, "", content);
	return true;
}

bool ChatWindow::onWindowResize(const WindowResizeEvent& event) {

	m_position.y = event.height - 30 - m_size.y;
	
	return false;
}

bool ChatWindow::onPlayerJoined(const NetworkJoinedEvent& event) {

	addMessage(255, "", event.player.name + " joined the game!");
	return true;
}



bool ChatWindow::onPlayerDisconnected(const NetworkDisconnectEvent& event) {

	auto it = m_messages.begin();
	while (it != m_messages.end()) {
		if (it->id == event.player.id) {
			it->id = char(254);
		}
		it++;
	}

	addMessage(255, "", event.player.name + (event.reason == PlayerLeftReason::KICKED ? " was kicked!" : " left the game!"));
	return true;
}