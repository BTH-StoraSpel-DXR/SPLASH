#include "LobbyHostState.h"
#include "Network/NWrapper.h"
#include "Network/NWrapperHost.h"
#include "Network/NWrapperSingleton.h"

#include "Sail/events/EventDispatcher.h"

LobbyHostState::LobbyHostState(StateStack& stack)
	: LobbyState(stack) {

	MatchRecordSystem* mrs = NWrapperSingleton::getInstance().recordSystem;
	if (!(mrs && mrs->status == 2)) {
		NWrapperSingleton::getInstance().setPlayerID(HOST_ID); 
		NWrapperSingleton::getInstance().startUDP();
	}

	m_isHost = true;

	if (!mrs) {
		mrs = new MatchRecordSystem();
		mrs->status = 0;
	}

}

LobbyHostState::~LobbyHostState() {

}
