
#include "Sail/../../Sail/src/Network/NetworkModule.hpp"
#include "MenuState.h"

#include "Sail.h"
#include "../libraries/imgui/imgui.h"

#include "Network/NWrapperSingleton.h"
#include "Network/NWrapper.h"

#include "Sail/../../SPLASH/src/game/events/NetworkLanHostFoundEvent.h"

#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include <string>



MenuState::MenuState(StateStack& stack) 
	: State(stack)
{
	m_input = Input::GetInstance();
	m_network = &NWrapperSingleton::getInstance();
	m_app = Application::getInstance();

	this->inputIP = SAIL_NEW char[100]{ "127.0.0.1:54000" };
	
	m_ipBuffer = SAIL_NEW char[m_ipBufferSize];
}

MenuState::~MenuState() {
	delete[] this->inputIP;
	delete[] m_ipBuffer;
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
	NWrapperSingleton::getInstance().checkFoundPackages();
	
	removeDeadLobbies();
	return false;
}

bool MenuState::render(float dt, float alpha) {
	m_app->getAPI()->clear({ 0.1f, 0.2f, 0.3f, 1.0f });
	ECS::Instance()->getSystem<BeginEndFrameSystem>()->renderNothing();
	return false;
}

bool MenuState::renderImgui(float dt) {
	
	// Host
	if(ImGui::Begin("Main Menu")) {
		ImGui::Text("Name:");
		ImGui::SameLine();
		ImGui::InputText("##name", &NWrapperSingleton::getInstance().getMyPlayerName().front(), MAX_NAME_LENGTH);
		ImGui::Separator();
		if (ImGui::Button("Single Player")) {
			if (m_network->host()) {
				NWrapperSingleton::getInstance().setPlayerID(HOST_ID);
				if (NWrapperSingleton::getInstance().getPlayers().size() == 0) {
					NWrapperSingleton::getInstance().playerJoined(NWrapperSingleton::getInstance().getMyPlayer());
				}

				m_app->getStateStorage().setLobbyToGameData(LobbyToGameData(0));

				this->requestStackPop();
				this->requestStackPush(States::Game);
			}
		}
		ImGui::SameLine(200);
		if (ImGui::Button("Host Game")) {
			if (m_network->host()) {
				// Update server description after host added himself to the player list.
				NWrapperSingleton::getInstance().getMyPlayer().id = HOST_ID;;
				NWrapperSingleton::getInstance().playerJoined(NWrapperSingleton::getInstance().getMyPlayer());

				NWrapperHost* wrapper = static_cast<NWrapperHost*>(NWrapperSingleton::getInstance().getNetworkWrapper());
				wrapper->setLobbyName(NWrapperSingleton::getInstance().getMyPlayer().name.c_str());

				this->requestStackPop();
				this->requestStackPush(States::HostLobby);
			}
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("IP:");
		ImGui::SameLine();
		ImGui::InputText("##IP:", inputIP, 100);
		if (ImGui::Button("Join Local Game")) {
			if (m_network->connectToIP(inputIP)) {
				// Wait until welcome-package is received,
				// Save the package info,
				// Pop and push into JoinLobbyState.
				this->requestStackPop();
				this->requestStackPush(States::JoinLobby);
			}
		}

	}
	ImGui::End();


	// Display open lobbies
	ImGui::Begin("Hosted Lobbies on LAN", NULL);
	// Per hosted game
	for (auto& lobby : m_foundLobbies) {
		// List as a button

		if (ImGui::Button((lobby.description != "") ? lobby.description.c_str() : lobby.ip.c_str())) {
			char* tempIp = SAIL_NEW char[m_ipBufferSize];
			tempIp = std::strcpy(tempIp, lobby.ip.c_str());

			// If pressed then join
			if (m_network->connectToIP(tempIp)) {
				this->requestStackPop();
				this->requestStackPush(States::JoinLobby);
				delete[] tempIp;
			}
		}
	}
	ImGui::End();


	return false;
}

bool MenuState::onEvent(Event& event) {
	EventHandler::dispatch<NetworkLanHostFoundEvent>(event, SAIL_BIND_EVENT(&MenuState::onLanHostFound));

	return false;
}

bool MenuState::onLanHostFound(NetworkLanHostFoundEvent& event) {
	// Get IP (as int) then convert into char*
	ULONG ip_as_int = event.getIp();
	Network::ip_int_to_ip_string(ip_as_int, m_ipBuffer, m_ipBufferSize);
	std::string ip_as_string(m_ipBuffer);

	// Get Port as well
	USHORT hostPort = event.getHostPort();
	ip_as_string += ":";
	ip_as_string.append(std::to_string(hostPort));

	// Check if it is already logged.	
	bool alreadyExists = false;
	for (auto& lobby : m_foundLobbies) {
		if (lobby.ip == ip_as_string) {
			alreadyExists = true;
			lobby.description = event.getDesc();
			lobby.resetDuration();
		}
	}
	// If not...
	if (alreadyExists == false) {
		// ...log it.
		m_foundLobbies.push_back(FoundLobby{ ip_as_string, event.getDesc() });
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
			if (lobbyToRemove.ip == lobby.ip) {
				m_foundLobbies.erase(m_foundLobbies.begin()+index);
				break;
			}
			index++;
		}
	}
}
