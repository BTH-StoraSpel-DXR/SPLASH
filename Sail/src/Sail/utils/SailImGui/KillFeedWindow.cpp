#include "pch.h"
#include "KillFeedWindow.h"

#include "Sail/Application.h"
#include "Sail/utils/GameDataTracker.h"

#include "Sail/events/EventDispatcher.h"
#include "Sail/events/types/PlayerDiedEvent.h"
#include "Sail/netcode/NetworkedStructs.h"
#include "Sail/../Network/NWrapperSingleton.h"

KillFeedWindow::KillFeedWindow(bool showWindow)
	: m_gameDataTracker(GameDataTracker::getInstance())
	, m_maxTimeShowed(10.f)
	, m_doRender(false) {
	EventDispatcher::Instance().subscribe(Event::Type::PLAYER_DEATH, this);
	EventDispatcher::Instance().subscribe(Event::Type::TORCH_EXTINGUISHED, this);
}

KillFeedWindow::~KillFeedWindow() {
	EventDispatcher::Instance().unsubscribe(Event::Type::PLAYER_DEATH, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::TORCH_EXTINGUISHED, this);
}


void KillFeedWindow::renderWindow() {
	if (m_doRender) {
		ImVec2 minSize = ImVec2(0.f, 0.f);
		ImVec2 maxSize = ImVec2(240.f, 90.f);

		ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.05f));
		ImGui::Begin("Kill Feed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		float alpha = 1.f;
		bool nothingToDisplay = true;
		for (int i = static_cast<int>(m_kills.size()) - 1; i > -1; i--) {
			alpha = m_maxTimeShowed - m_kills[i].first;
			alpha = Utils::clamp(alpha, 0.f, 1.f);
			if (alpha > 0.f) {
				auto name1Color = ImVec4(0.9882f, 0.6941f, 0.0117f, alpha);
				auto name2Color = name1Color;
				auto typeColor = ImVec4(0.8f, 0.8f, 0.8f, alpha);
				if (m_kills[i].second.relevant == 1) {
					name1Color.x = 0.3215f;
					name1Color.y = 0.6705f;
					name1Color.z = 0.8902f;
				} else if (m_kills[i].second.relevant == 2) {
					name2Color.x = 0.3215f;
					name2Color.y = 0.6705f;
					name2Color.z = 0.8902f;
				}

				ImGui::PushStyleColor(ImGuiCol_Text, name1Color);
				ImGui::Text(m_kills[i].second.name1.c_str());
				ImGui::PopStyleColor(1);

				ImGui::PushStyleColor(ImGuiCol_Text, typeColor);
				ImGui::SameLine();
				ImGui::Text(m_kills[i].second.type.c_str());
				ImGui::PopStyleColor(1);

				ImGui::PushStyleColor(ImGuiCol_Text, name2Color);
				ImGui::SameLine();
				ImGui::Text(m_kills[i].second.name2.c_str());
				ImGui::PopStyleColor(1);

				nothingToDisplay = false;
			}
		}
		m_doRender = !nothingToDisplay;

		ImVec2 size = ImGui::GetWindowSize();
		ImVec2 pos = ImVec2(Application::getInstance()->getWindow()->getWindowWidth() - size.x, 0.f);
		ImGui::SetWindowPos(pos);

		ImGui::End();
		ImGui::PopStyleColor(1);
	}
}

// Update all timings used in the window, also fetches the deaths
void KillFeedWindow::updateTiming(float dt) {
	std::vector<unsigned int> toRemove;
	auto it = m_kills.begin();
	while (it != m_kills.end()) {
		it->first += dt;
		if (it->first > m_maxTimeShowed) {
			it = m_kills.erase(it);
		} else {
			it++;
		}
	}
}

bool KillFeedWindow::onEvent(const Event& event) {
	auto onPlayerDied = [&](const PlayerDiedEvent& e) {

		Netcode::PlayerID idOfDeadPlayer = Netcode::getComponentOwner(e.netIDofKilled);
		KillFeedWindow::KillFeedInfo killFeedInfo;
		killFeedInfo.name2 = NWrapperSingleton::getInstance().getPlayer(idOfDeadPlayer)->name;

		if (e.killerID == Netcode::SPRINKLER_COMP_ID) {
			killFeedInfo.name1 = "The sprinklers";
			killFeedInfo.type = "eliminated";
		} else if (e.killerID == Netcode::INSANITY_COMP_ID) {
			killFeedInfo.name1 = "Insanity";
			killFeedInfo.type = "devoured";
		} else if (Netcode::getComponentOwner(e.killerID) == idOfDeadPlayer) {
			killFeedInfo.name1 = killFeedInfo.name2;
			killFeedInfo.type = "eliminated himself!";
			killFeedInfo.name2 = "";
		} else {
			killFeedInfo.name1 = NWrapperSingleton::getInstance().getPlayer(Netcode::getComponentOwner(e.killerID))->name;
			killFeedInfo.type = "eliminated";
		}

		SAIL_LOG(killFeedInfo.name1 + " " + killFeedInfo.type + " " + killFeedInfo.name2);
		auto myID = NWrapperSingleton::getInstance().getMyPlayerID();
		if (Netcode::getComponentOwner(e.killerID) == myID) {
			killFeedInfo.relevant = 1;
		} else if (idOfDeadPlayer == myID) {
			killFeedInfo.relevant = 2;
		}

		m_kills.emplace_back(0.f, killFeedInfo);
		m_doRender = true;

	};

	auto onTorchExtinguished = [&] (const TorchExtinguishedEvent& e) {
		auto extinguishedOwner = NWrapperSingleton::getInstance().getPlayer(Netcode::getComponentOwner(e.netIDextinguished));

		KillFeedWindow::KillFeedInfo killFeedInfo;
		if (e.shooterID == Netcode::MESSAGE_SPRINKLER_ID) {
			killFeedInfo.name1 = "The sprinklers";
			killFeedInfo.type = "sprayed down";
			killFeedInfo.name2 = extinguishedOwner->name;
		} else if (e.shooterID == Netcode::MESSAGE_INSANITY_ID) {
			killFeedInfo.name1 = extinguishedOwner->name;
			killFeedInfo.type = "got spooked";
			killFeedInfo.name2 = "";
		} else if (e.shooterID == extinguishedOwner->id) {
			killFeedInfo.name1 = extinguishedOwner->name;
			killFeedInfo.type = "sprayed down himself!";
			killFeedInfo.name2 = "";
		} else {
			killFeedInfo.name1 = NWrapperSingleton::getInstance().getPlayer(e.shooterID)->name;
			killFeedInfo.type = "sprayed down";
			killFeedInfo.name2 = extinguishedOwner->name;
		}




		std::string message = killFeedInfo.name1 + " " + killFeedInfo.type + " " + killFeedInfo.name2;
		SAIL_LOG(message);

		auto myID = NWrapperSingleton::getInstance().getMyPlayerID();
		if (e.shooterID == myID) {
			killFeedInfo.relevant = 1;
		}
		else if (extinguishedOwner->id == myID && e.shooterID == Netcode::MESSAGE_INSANITY_ID) {
			killFeedInfo.relevant = 1;
		}
		else if (extinguishedOwner->id == myID) {
			killFeedInfo.relevant = 2;
		}
		

		m_kills.emplace_back(0.f, killFeedInfo);
		m_doRender = true;
	};

	switch (event.type) {
	case Event::Type::TORCH_EXTINGUISHED: onTorchExtinguished((const TorchExtinguishedEvent&)event); break;
	case Event::Type::PLAYER_DEATH: onPlayerDied((const PlayerDiedEvent&)event); break;
	default: break;
	}

	return true;
}
