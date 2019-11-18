#pragma once

#include "LobbyState.h"

#include "../SPLASH/src/game/events/TextInputEvent.h"
#include "../SPLASH/src/game/events/NetworkNameEvent.h"

class LobbyHostState final : public LobbyState {
public:
	LobbyHostState(StateStack& stack);
	~LobbyHostState();
private:
	bool onMyTextInput(const TextInputEvent& event);
};