#pragma once

#include "LobbyState.h"

#include "../SPLASH/src/game/events/TextInputEvent.h"
#include "../SPLASH/src/game/events/NetworkChatEvent.h"
#include "../SPLASH/src/game/events/NetworkJoinedEvent.h"
#include "../SPLASH/src/game/events/NetworkDisconnectEvent.h"
#include "../events/NetworkWelcomeEvent.h"

class LobbyHostState : public LobbyState {
public:
	LobbyHostState(StateStack& stack);
	~LobbyHostState();

	bool onEvent(Event& event);

private:
	bool processInput(float dt);
	bool onMyTextInput(TextInputEvent& event);
	bool onRecievedText(NetworkChatEvent& event);
	bool onPlayerJoined(NetworkJoinedEvent& event);
	bool onPlayerDisconnected(NetworkDisconnectEvent& event);

};