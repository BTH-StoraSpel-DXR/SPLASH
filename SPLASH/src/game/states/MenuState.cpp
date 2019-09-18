#include "MenuState.h"

#include "Sail.h"
#include "Network/NetworkWrapper.h"
#include "../libraries/imgui/imgui.h"

MenuState::MenuState(StateStack& stack) 
	: State(stack)
{
	m_input = Input::GetInstance();
	m_network = &NetworkWrapper::getInstance();
	m_app = Application::getInstance();

	this->inputIP = new char[100]{ "127.0.0.1:54000" };
	this->inputName = new char[100]{ "Gottem420" };
}

MenuState::~MenuState() {
	delete this->inputIP;
	delete this->inputName;
}

bool MenuState::processInput(float dt) {
	//// TEMPORARY TESTING FOR NETWORK
	//if (NetworkWrapper::getInstance().isInitialized())
	//{
	//	NetworkWrapper::getInstance().checkForPackages();
	//}

	//// -------- HOST -----------
	//if (Input::GetInstance()->IsKeyPressed(SAIL_KEY_H)) {

	//	if (NetworkWrapper::getInstance().host())
	//	{
	//		printf("Setting up host.");
	//		
	//		this->requestStackPop();
	//		this->requestStackPush(States::HostLobby);
	//	}
	//	else
	//	{
	//		printf("Failed to set up Host.");
	//	}
	//}
	//// -------- JOIN -----------
	//if (Input::GetInstance()->IsKeyPressed(SAIL_KEY_J))
	//{

	//	printf("Attempting connection... \n");

	//	if (NetworkWrapper::getInstance().connectToIP("127.0.0.1:54000"))
	//	{
	//		printf("Connecting to 192.168.1.55. \n");
	//		this->requestStackPop();
	//		this->requestStackPush(States::JoinLobby);
	//	}
	//	else
	//	{
	//		printf("Failed to connect. \n");
	//	}
	//}
	//// -------- MESSAGE -----------
	//if (Input::GetInstance()->IsKeyPressed(SAIL_KEY_M))
	//{
	//	NetworkWrapper::getInstance().sendChatMsg("Kanel finns nu.");
	//}

	return false;
}

bool MenuState::update(float dt) {
	return false;
}

bool MenuState::render(float dt) {
	m_app->getAPI()->clear({ 0.1f, 0.2f, 0.3f, 1.0f });
	m_scene.draw();
	return false;
}

bool MenuState::renderImgui(float dt) {
	
	// Host
	ImGui::Begin("Host Game");
	if (ImGui::Button("S.P.L.A.S.H over here")) {
		if (m_network->host()) {
			printf("Setting up host.");
			this->requestStackPop();
			this->requestStackPush(States::HostLobby);
		}
	}
	ImGui::End();

	ImGui::Begin("Name:");
	ImGui::InputText("", inputName, 100);
	ImGui::End();

	// 
	ImGui::Begin("Join Game");
	ImGui::InputText("IP:", inputIP, 100);
	if (ImGui::Button("S.P.L.A.S.H over there")) {
		if (m_network->connectToIP(inputIP)) {
			printf("Connecting to 192.168.1.55. \n");

			// Wait until welcome-package is recieved,
			// Save the package info,
			// Pop and push into JoinLobbyState.

			this->requestStackPop();
			this->requestStackPush(States::JoinLobby);
		}
	}
	ImGui::End();



	return false;
}
