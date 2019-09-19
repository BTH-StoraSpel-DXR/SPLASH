#include "LobbyHostState.h"
#include "../libraries/imgui/imgui.h"

LobbyHostState::LobbyHostState(StateStack& stack)
	: LobbyState(stack)
{

}

LobbyHostState::~LobbyHostState() {
}

bool LobbyHostState::onEvent(Event& event)
{
	EventHandler::dispatch<TextInputEvent>(event, SAIL_BIND_EVENT(&LobbyHostState::onMyTextInput));
	EventHandler::dispatch<NetworkChatEvent>(event, SAIL_BIND_EVENT(&LobbyHostState::onRecievedText));
	EventHandler::dispatch<NetworkJoinedEvent>(event, SAIL_BIND_EVENT(&LobbyHostState::onPlayerJoined));
	EventHandler::dispatch<NetworkDisconnectEvent>(event, SAIL_BIND_EVENT(&LobbyHostState::onPlayerDisconnected));

	return false;
}

bool LobbyHostState::processInput(float dt)
{
	if (m_input->IsMouseButtonPressed(0)) {
		m_chatFocus = false;
	}

	return false;
}

bool LobbyHostState::onMyTextInput(TextInputEvent& event) {
	// Add to current message, If 'enter' ...
	if (this->inputToChatLog(event.getMSG())) {
		// ... Add current message to chat log
		this->addTextToChat(&std::string(m_currentmessage));

		// ... Send the message to other clients and reset message
		m_network->sendChatAllClients(this->fetchMessage());
	}

	return true;
}

bool LobbyHostState::onRecievedText(NetworkChatEvent& event) {
	// Add recieved text to chat log
	this->addTextToChat(&event.getMessage());

	// Send out the recieved text to joined players
	// --- Wrapper already does this

	return false;
}

bool LobbyHostState::onPlayerJoined(NetworkJoinedEvent& event) {
	// Add player to player list
	this->playerJoined(event.getPlayer());

	// Send out 'playerjoined'
	// --- Wrapper already does this

	// Send a welcome package to all players
	std::string listOfPlayers = "w";
	for (auto currentPlayer : m_players) {
		listOfPlayers.append(currentPlayer.name).append(":");
	}
	m_network->sendMsgAllClients(listOfPlayers);

	return true;
}

bool LobbyHostState::onPlayerDisconnected(NetworkDisconnectEvent& event) {
	// Remove player from player list
	this->playerLeft(event.getPlayerID());

	// Send out 'playerdisconnected'
	// --- Wrapper already does this
	
	return false;
}

