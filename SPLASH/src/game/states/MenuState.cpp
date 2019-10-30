
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

	std::string name = loadPlayerName("res/data/localplayer.settings");
	m_network->setPlayerName(name.c_str());
	this->inputIP = SAIL_NEW char[100]{ "127.0.0.1:54000" };
	
	m_ipBuffer = SAIL_NEW char[m_ipBufferSize];

	m_modelThread = m_app->pushJobToThreadPool([&](int id) {return loadModels(m_app); });
}

MenuState::~MenuState() {
	delete[] this->inputIP;
	delete[] m_ipBuffer;
	
	Utils::writeFileTrunc("res/data/localplayer.settings", NWrapperSingleton::getInstance().getMyPlayerName());
	m_modelThread.get();
}

bool MenuState::processInput(float dt) {
	return false;
}

bool MenuState::update(float dt, float alpha) {
	udpCounter -= m_frameTick;
	if (udpCounter < 0) {
		NWrapperSingleton::getInstance().searchForLobbies();
		NWrapperSingleton::getInstance().checkFoundPackages(); // Make timer.
		udpCounter = udpChill;
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
	
	ImGui::ShowDemoWindow();


	static char buf[101] = "";
	// Host
	if(ImGui::Begin("Main Menu")) {
		ImGui::Text("Name:");
		ImGui::SameLine();
		std::string name = &NWrapperSingleton::getInstance().getMyPlayerName().front();
		
		strncpy_s(buf, name.c_str(), name.size());
		ImGui::InputText("##name", buf, MAX_NAME_LENGTH);
		name = buf;
		NWrapperSingleton::getInstance().setPlayerName(name.c_str());
		
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
		if (ImGui::Button(lobby.ip.c_str())) {
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





	if (ImGui::Begin("Loading Info")) {
		//maxCount being hardcoded for now
		std::string progress = "Models (" + std::to_string(m_app->getResourceManager().numberOfModels()) + "/"+"14"+")";
		ImGui::Text(progress.c_str());

		progress = "Textures (" + std::to_string(m_app->getResourceManager().numberOfTextures()) + "/" + "44" + ")";
		ImGui::Text(progress.c_str());



	}
	ImGui::End();
	return false;
}

bool MenuState::onEvent(Event& event) {
	EventHandler::dispatch<NetworkLanHostFoundEvent>(event, SAIL_BIND_EVENT(&MenuState::onLanHostFound));

	return false;
}

const std::string MenuState::loadPlayerName(const std::string& file) {
	std::string name = Utils::readFile(file);
	if (name == "") {
		name = "Hans";
		Utils::writeFileTrunc("res/data/localplayer.settings", name);
		Logger::Log("Found no player file, created: " + std::string("'res/data/localplayer.settings'"));
	}
	return name;
}

bool MenuState::loadModels(Application* app) {
	ResourceManager* rm = &app->getResourceManager();


	rm->setDefaultShader(&app->getResourceManager().getShaderSet<GBufferOutShader>());
	rm->loadModel("Doc.fbx");
	rm->loadModel("candleExported.fbx");
	rm->loadModel("Tiles/tileFlat.fbx");
	rm->loadModel("Tiles/RoomWall.fbx");
	rm->loadModel("Tiles/tileDoor.fbx");
	rm->loadModel("Tiles/RoomDoor.fbx");
	rm->loadModel("Tiles/CorridorDoor.fbx");
	rm->loadModel("Tiles/CorridorWall.fbx");
	rm->loadModel("Tiles/RoomCeiling.fbx");
	rm->loadModel("Tiles/CorridorFloor.fbx");
	rm->loadModel("Tiles/RoomFloor.fbx");
	rm->loadModel("Tiles/CorridorCeiling.fbx");
	rm->loadModel("Tiles/CorridorCorner.fbx");
	rm->loadModel("Tiles/RoomCorner.fbx");
	rm->loadModel("Tiles/SmallObject.fbx");
	rm->loadModel("Tiles/MediumObject.fbx");
	rm->loadModel("Tiles/LargeObject.fbx");

	rm->loadTexture("pbr/Tiles/RoomWallMRAO.tga");
	rm->loadTexture("pbr/Tiles/RoomWallNM.tga");
	rm->loadTexture("pbr/Tiles/RoomWallAlbedo.tga");

	rm->loadTexture("pbr/Tiles/RD_MRAo.tga");
	rm->loadTexture("pbr/Tiles/RD_NM.tga");
	rm->loadTexture("pbr/Tiles/RD_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CD_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CD_NM.tga");
	rm->loadTexture("pbr/Tiles/CD_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CW_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CW_NM.tga");
	rm->loadTexture("pbr/Tiles/CW_Albedo.tga");

	rm->loadTexture("pbr/Tiles/F_MRAo.tga");
	rm->loadTexture("pbr/Tiles/F_NM.tga");
	rm->loadTexture("pbr/Tiles/F_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CF_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CF_NM.tga");
	rm->loadTexture("pbr/Tiles/CF_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CC_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CC_NM.tga");
	rm->loadTexture("pbr/Tiles/CC_Albedo.tga");

	rm->loadTexture("pbr/Tiles/RC_MRAo.tga");
	rm->loadTexture("pbr/Tiles/RC_NM.tga");
	rm->loadTexture("pbr/Tiles/RC_Albedo.tga");

	rm->loadTexture("pbr/Tiles/Corner_MRAo.tga");
	rm->loadTexture("pbr/Tiles/Corner_NM.tga");
	rm->loadTexture("pbr/Tiles/Corner_Albedo.tga");

	rm->loadTexture("pbr/metal/metalnessRoughnessAO.tga");
	rm->loadTexture("pbr/metal/normal.tga");
	rm->loadTexture("pbr/metal/albedo.tga");

	rm->loadTexture("pbr/Clutter/LO_MRAO.tga");
	rm->loadTexture("pbr/Clutter/LO_NM.tga");
	rm->loadTexture("pbr/Clutter/LO_Albedo.tga");

	rm->loadTexture("pbr/Clutter/MO_MRAO.tga");
	rm->loadTexture("pbr/Clutter/MO_NM.tga");
	rm->loadTexture("pbr/Clutter/MO_Albedo.tga");

	rm->loadTexture("pbr/Clutter/SO_MRAO.tga");
	rm->loadTexture("pbr/Clutter/SO_NM.tga");
	rm->loadTexture("pbr/Clutter/SO_Albedo.tga");


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
			lobby.resetDuration();
		}
	}
	// If not...
	if (alreadyExists == false) {
		// ...log it.
		m_foundLobbies.push_back(FoundLobby{ ip_as_string });
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
