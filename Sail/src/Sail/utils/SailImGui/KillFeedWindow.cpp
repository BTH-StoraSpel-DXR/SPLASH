#include "pch.h"
#include "KillFeedWindow.h"

#include "Sail/Application.h"
#include "Sail/utils/GameDataTracker.h"

KillFeedWindow::KillFeedWindow(bool showWindow)
	: m_gameDataTracker(GameDataTracker::getInstance())
	, m_maxTimeShowed(3.f)
	, m_doRender(false)
{}

KillFeedWindow::~KillFeedWindow() {}

void KillFeedWindow::renderWindow() {
	if (m_doRender) {
		ImVec2 minSize = ImVec2(0.f, 0.f);
		ImVec2 maxSize = ImVec2(240.f, 90.f);

		ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.05f));
		ImGui::Begin("Kill Feed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		float alpha = 1.f;
		bool nothingToDisplay = true;
		for (int i = m_kills.size() - 1; i > -1; i--) {
			alpha = m_maxTimeShowed - m_kills[i].first;
			alpha = Utils::clamp(alpha, 0.f, 1.f);
			if (alpha > 0.f) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.7f, 0.7f, alpha));
				ImGui::Text(m_kills[i].second.c_str());
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
	for (auto& kill : m_kills) {
		kill.first += dt;
	}

	auto allDeaths = m_gameDataTracker.getPlayerDeaths();
	if (m_kills.size() < allDeaths.size()) {
		for (int i = m_kills.size(); i < allDeaths.size(); i++) {
			m_kills.emplace_back(0.f, allDeaths[i]);
			m_doRender = true;
		}
	}
}
