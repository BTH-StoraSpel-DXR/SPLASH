#include "pch.h"
#include "NetworkReceiverSystemHost.h"
#include "../NetworkSenderSystem.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/Utils.h"
#include "Sail/utils/GameDataTracker.h"
#include "Sail/events/types/StartKillCamEvent.h"
#include "Sail/events/types/StopKillCamEvent.h"

#include "../SPLASH/src/game/states/GameState.h"

NetworkReceiverSystemHost::NetworkReceiverSystemHost() {
	EventDispatcher::Instance().subscribe(Event::Type::START_KILLCAM, this);
	EventDispatcher::Instance().subscribe(Event::Type::STOP_KILLCAM, this);
}

NetworkReceiverSystemHost::~NetworkReceiverSystemHost() {
	EventDispatcher::Instance().unsubscribe(Event::Type::START_KILLCAM, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::STOP_KILLCAM, this);
}


void NetworkReceiverSystemHost::handleIncomingData(const std::string& data) {
	pushDataToBuffer(data);

	// The host will also save the data in the sender system so that it can be forwarded to all other clients
	m_netSendSysPtr->pushDataToBuffer(data);
}

void NetworkReceiverSystemHost::stop() {
	m_startEndGameTimer = false;
	m_finalKillCamOver = true;
}

#ifdef DEVELOPMENT
unsigned int NetworkReceiverSystemHost::getByteSize() const {
	return BaseComponentSystem::getByteSize() + sizeof(*this);
}
#endif

void NetworkReceiverSystemHost::endGame() {
	m_startEndGameTimer = true;
}


// HOST ONLY FUNCTIONS

void NetworkReceiverSystemHost::endMatchAfterTimer(const float dt) {
	static float endGameClock = 0.f;

	if (m_startEndGameTimer) {
		endGameClock += dt;
	}
 
	// Wait until the final killcam has ended or until 2 seconds has passed
	if (m_finalKillCamOver && endGameClock > 2.0f) {
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
	endGame(); // Starts the end game timer. Runs only for the host

}

bool NetworkReceiverSystemHost::onEvent(const Event& event) {
	NetworkReceiverSystem::onEvent(event);

	switch (event.type) {
	case Event::Type::START_KILLCAM:
		if (((const StartKillCamEvent&)event).finalKillCam) {
 			m_finalKillCamOver = false;
		}
		break;
	case Event::Type::STOP_KILLCAM:
		if (((const StopKillCamEvent&)event).isFinalKill) {
			m_finalKillCamOver = true;
		}
		break;
	default: 
		break;
	}

	return true;
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

	endGame();
}
