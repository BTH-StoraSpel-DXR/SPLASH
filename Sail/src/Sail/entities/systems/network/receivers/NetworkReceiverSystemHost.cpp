#include "pch.h"
#include "NetworkReceiverSystemHost.h"
#include "../NetworkSenderSystem.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/Utils.h"
#include "Sail/utils/GameDataTracker.h"

#include "../SPLASH/src/game/states/GameState.h"

NetworkReceiverSystemHost::NetworkReceiverSystemHost() 
{}

NetworkReceiverSystemHost::~NetworkReceiverSystemHost() 
{}


void NetworkReceiverSystemHost::handleIncomingData(const std::string& data) {
	pushDataToBuffer(data);

	// The host will also save the data in the sender system so that it can be forwarded to all other clients
	m_netSendSysPtr->pushDataToBuffer(data);
}

void NetworkReceiverSystemHost::endMatch() {
	m_startEndGameTimer = true;
}


// HOST ONLY FUNCTIONS

void NetworkReceiverSystemHost::endMatchAfterTimer(const float dt) {
	static float endGameClock = 0.f;

	if (m_startEndGameTimer) {
		endGameClock += dt;
	}
	if (endGameClock > 2.0f) {
		NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
			Netcode::MessageType::ENDGAME_STATS,
			nullptr,
			false
		);

		m_gameStatePtr->requestStackClear();
		m_gameStatePtr->requestStackPush(States::EndGame);

		// Reset clock for next session
		m_startEndGameTimer = false;
		endGameClock = 0.f;
	}
}

void NetworkReceiverSystemHost::prepareEndScreen(const Netcode::PlayerID sender, const EndScreenInfo& info) {

	GlobalTopStats* gts = &GameDataTracker::getInstance().getStatisticsGlobal();

	// Process the data
	if (info.bulletsFired > gts->bulletsFired) {
		gts->bulletsFired = info.bulletsFired;
		gts->bulletsFiredID = sender;
	}
	if (info.distanceWalked > gts->distanceWalked) {
		gts->distanceWalked = info.distanceWalked;
		gts->distanceWalkedID = sender;
	}
	if (info.jumpsMade > gts->jumpsMade) {
		gts->jumpsMade = info.jumpsMade;
		gts->jumpsMadeID = sender;
	}

	// Send data back in Netcode::MessageType::ENDGAME_STATS
	endMatch(); // Starts the end game timer. Runs only for the host

}

void NetworkReceiverSystemHost::mergeHostsStats() {

	GameDataTracker* gdt = &GameDataTracker::getInstance();
	Netcode::PlayerID id = NWrapperSingleton::getInstance().getMyPlayerID();

	if (gdt->getStatisticsLocal().bulletsFired > gdt->getStatisticsGlobal().bulletsFired) {
		gdt->getStatisticsGlobal().bulletsFired = gdt->getStatisticsLocal().bulletsFired;
		gdt->getStatisticsGlobal().bulletsFiredID = id;
	}
	if (gdt->getStatisticsLocal().distanceWalked > gdt->getStatisticsGlobal().distanceWalked) {
		gdt->getStatisticsGlobal().distanceWalked = gdt->getStatisticsLocal().distanceWalked;
		gdt->getStatisticsGlobal().distanceWalkedID = id;
	}
	if (gdt->getStatisticsLocal().jumpsMade > gdt->getStatisticsGlobal().jumpsMade) {
		gdt->getStatisticsGlobal().jumpsMade = gdt->getStatisticsLocal().jumpsMade;
		gdt->getStatisticsGlobal().jumpsMadeID = id;
	}
}
