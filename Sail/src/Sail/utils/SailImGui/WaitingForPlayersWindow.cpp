#include "pch.h"
#include "WaitingForPlayersWindow.h"
#include "imgui.h"
#include "Network/NWrapperSingleton.h"

WaitingForPlayersWindow::WaitingForPlayersWindow(bool showWindow) {
}

WaitingForPlayersWindow::~WaitingForPlayersWindow() {
}

void WaitingForPlayersWindow::renderWindow() {

	ImGui::SetNextWindowSize(ImVec2(400,400));
	ImGui::Begin("Waiting For Players", NULL, ImGuiWindowFlags_NoCollapse);
	for (auto p : NWrapperSingleton::getInstance().getPlayers()) {
		if (p.lastStateStatus.state != state || p.lastStateStatus.status < minStatus) {
			ImGui::Text(p.name.c_str());
		}
	}	
	ImGui::End();

}

void WaitingForPlayersWindow::setStateStatus(States::ID state, char status) {
	this->state = state;
	this->minStatus = status;
}
