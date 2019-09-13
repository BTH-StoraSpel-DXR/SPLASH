#include "Game.h"
#include "states/GameState.h"
#include "states/LobbyState.h"
#include "states/MenuState.h"
#include "states/BobbyHostState.h"
#include "Network/NetworkWrapper.h"

Game::Game(HINSTANCE hInstance)
	: Application(1280, 720, "Sail | Game Engine Demo", hInstance)
	, m_stateStack()
	
{
	// Register states
	registerStates();

	// Set starting state
	m_stateStack.pushState(States::HostLobby);
	
	// Initialize the Network wrapper instance.
	NetworkWrapper::getInstance().Initialize();
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
	m_stateStack.registerState<BobbyHostState>(States::HostLobby);
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