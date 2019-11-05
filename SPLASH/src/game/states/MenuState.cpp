
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
	: State(stack),
	inputIP("")
{
	m_input = Input::GetInstance();
	m_network = &NWrapperSingleton::getInstance();
	m_app = Application::getInstance();

	std::string name = loadPlayerName("res/data/localplayer.settings");
	m_network->setPlayerName(name.c_str());
	
	m_ipBuffer = SAIL_NEW char[m_ipBufferSize];

	m_modelThread = m_app->pushJobToThreadPool([&](int id) {return loadModels(m_app); });
}

MenuState::~MenuState() {
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
	
	//Keep
	//ImGui::ShowDemoWindow();


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
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		if (ImGui::CollapsingHeader("SinglePlayer", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Spacing();
			ImGui::Spacing();
			if (ImGui::Button("Start Solo Game")) {
				if (m_network->host()) {
					NWrapperSingleton::getInstance().setPlayerID(HOST_ID);
					if (NWrapperSingleton::getInstance().getPlayers().size() == 0) {
						NWrapperSingleton::getInstance().playerJoined(NWrapperSingleton::getInstance().getMyPlayer());
					}

					m_app->getStateStorage().setLobbyToGameData(LobbyToGameData(0, false));

					this->requestStackPop();
					this->requestStackPush(States::Game);
				}
			}
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
		}
	
		if (ImGui::CollapsingHeader("MultiPlayer", ImGuiTreeNodeFlags_DefaultOpen)) {
			static std::string lobbyName = "";
			strncpy_s(buf, lobbyName.c_str(), lobbyName.size());
			ImGui::InputTextWithHint("##lobbyName:", std::string(name+"'s lobby").c_str() , buf, 40);
			lobbyName = buf;
			ImGui::SameLine();
			if (ImGui::Button("Host Game")) {
				if (m_network->host()) {
					// Update server description after host added himself to the player list.
					NWrapperSingleton::getInstance().getMyPlayer().id = HOST_ID;
					NWrapperSingleton::getInstance().playerJoined(NWrapperSingleton::getInstance().getMyPlayer());
					NWrapperHost* wrapper = static_cast<NWrapperHost*>(NWrapperSingleton::getInstance().getNetworkWrapper());
					if (lobbyName == "") {
						lobbyName = NWrapperSingleton::getInstance().getMyPlayer().name + "'s lobby";
					}
					wrapper->setLobbyName(lobbyName.c_str());

					this->requestStackPop();
					this->requestStackPush(States::HostLobby);
				}
			}
			ImGui::Separator();
			strncpy_s(buf, inputIP.c_str(), inputIP.size());
			ImGui::InputTextWithHint("##IP:","127.0.0.1:54000", buf, 100);
			inputIP = buf;
			ImGui::SameLine();
			if (ImGui::Button("Join")) {
				if (inputIP == "") {
					inputIP = "127.0.0.1:54000";
				}
				if (m_network->connectToIP(&inputIP.front())) {
					// Wait until welcome-package is received,
					// Save the package info,
					// Pop and push into JoinLobbyState.
					this->requestStackPop();
					this->requestStackPush(States::JoinLobby);
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				
				ImGui::Text("Leave empty to join local game");
				ImGui::EndTooltip();
			}
		}
	}
	ImGui::End();


	// Display open lobbies
	if (ImGui::Begin("Hosted Lobbies on LAN", NULL)) {
		
		// DISPLAY LOBBIES
		static int selected = -1;
		if (ImGui::BeginChild("Lobbies", ImVec2(0,-ImGui::GetFrameHeightWithSpacing()))) {
			// Per hosted game
			ImGui::Columns(2, "testColumns", true);
			ImGui::Separator();
			ImGui::Text("Lobby"); ImGui::NextColumn();
			ImGui::Text("Players"); ImGui::NextColumn();
			ImGui::Separator();

			int index = 0;
			if (selected >= m_foundLobbies.size()) {
				selected = -1;
			}
			for (auto& lobby : m_foundLobbies) {
				// List as a button
				std::string fullText = lobby.description;
				std::string lobbyName = "";
				std::string playerCount = "N/A";
				// GET LOBBY NAME AND PLAYERCOUNT AS SEPARATE STRINGS
				if (fullText == "") {
					lobbyName = lobby.ip;
				}
				else {
					int p0 = fullText.find_last_of("(");
					int p1 = fullText.find_last_of(")");
					lobbyName = fullText.substr(0, p0 - 1);
					playerCount = fullText.substr(p0 + 1, p1 - p0 - 1);
				}


				if (ImGui::Selectable(lobbyName.c_str(), selected == index, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
					
					selected = (index == selected ? -1 : index);
					if (ImGui::IsMouseDoubleClicked(0)) {
						// If pressed then join
						if (m_network->connectToIP(&lobby.ip.front())) {
							this->requestStackPop();
							this->requestStackPush(States::JoinLobby);
						}
					}
				}
				ImGui::NextColumn();
				ImGui::Text(playerCount.c_str()); ImGui::NextColumn();
				index++;
			}
		}
		ImGui::EndChild();

		// DISPLAY JOIN BUTTON
		ImGui::NewLine();
		ImGui::SameLine(ImGui::GetWindowWidth() - 50);
		if (selected == -1) {
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.3f);
		}
		if (ImGui::Button("Join") && selected != -1) {
			// If pressed then join
			if (m_network->connectToIP(&m_foundLobbies[selected].ip.front())) {
				this->requestStackPop();
				this->requestStackPush(States::JoinLobby);
			}
		}
		if (selected == -1) {
			ImGui::PopStyleVar();
		}
	}
	ImGui::End();
	


	if (ImGui::Begin("Loading Info")) {
		//maxCount being hardcoded for now
		std::string progress = "Models:";
		ImGui::Text(progress.c_str());
		ImGui::SameLine();
		ImGui::ProgressBar(m_app->getResourceManager().numberOfModels()/14.0f, ImVec2(0.0f, 0.0f));

		progress = "Textures:";
		ImGui::Text(progress.c_str());
		ImGui::SameLine();
		ImGui::ProgressBar(m_app->getResourceManager().numberOfTextures()/44.0f, ImVec2(0.0f, 0.0f));
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
	rm->loadModel("Clutter/SmallObject.fbx");
	rm->loadModel("Clutter/MediumObject.fbx");
	rm->loadModel("Clutter/LargeObject.fbx");

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
