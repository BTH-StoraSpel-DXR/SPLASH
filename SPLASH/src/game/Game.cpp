#include "Game.h"
#include "states/GameState.h"
#include "states/MenuState.h"
#include "states/LobbyHostState.h"
#include "states/LobbyJoinState.h"
#include "Network/NetworkWrapper.h"

Game::Game(HINSTANCE hInstance)
	: Application(1280, 720, "Sail | Game Engine Demo", hInstance)
	, m_stateStack()
	
{
	// Register states
	registerStates();

	// Set starting state
	m_stateStack.pushState(States::MainMenu);
	
	// Initialize the Network wrapper instance.
	NetworkWrapper::getInstance().initialize();
}

Game::~Game() {
}

int Game::run() {
	// Start the game loop and return when game exits
	return startGameLoop();
}

void Game::registerStates() {
	// Register all of the different states
	m_stateStack.registerState<GameState>(States::Game);
	m_stateStack.registerState<LobbyHostState>(States::HostLobby);
	m_stateStack.registerState<LobbyJoinState>(States::JoinLobby);
	m_stateStack.registerState<MenuState>(States::MainMenu);
}

void Game::dispatchEvent(Event& event) {
	Application::dispatchEvent(event);
	m_stateStack.onEvent(event);
}

void Game::processInput(float dt) {
	m_stateStack.processInput(dt);
}

void Game::update(float dt) {
	m_stateStack.update(dt);
}

void Game::render(float dt) {
	m_stateStack.render(dt);
}

//TCP_CONNECTION_ID NetworkWrapper::parseID(std::string& data) {
//	if (data.size() > 63) {
//		return 0;
//	}
//	if (data.size() < 1) {
//		return 0;
//	}
//	else {
//		// Remove opening ':' / '?' marker.
//		data.erase(0, 1);
//
//		std::string id_string = "";
//		int lastIndex;
//		for (lastIndex = 0; lastIndex < data.size(); lastIndex++) {
//			if (data[lastIndex] == '\0' || data[lastIndex] == ':') {
//				break;
//			}
//			else {
//				id_string += data[lastIndex];
//			}
//		}
//
//		data.erase(0, lastIndex);
//		if (id_string != "") {
//			return stoi(id_string);
//		}
//		else {
//			return 0;
//		}
//
//	}
//}
//
//std::string NetworkWrapper::parseName(std::string& data) {
//	if (data.size() < 1) {
//		return data;
//	}
//	else {
//		// Remove first ':' marker
//		data.erase(0, 1);
//
//		int lastIndex;
//		std::string parsedName = "";
//		for (lastIndex = 0; lastIndex < MAX_PACKAGE_SIZE; lastIndex++) {
//			if (data[lastIndex] == ':') { // Does parseID also remove the last ':'? no?
//				break;
//			}
//			else {
//				parsedName += data[lastIndex];
//			}
//		}
//
//		data.erase(0, lastIndex);
//		return parsedName;
//	}
//}