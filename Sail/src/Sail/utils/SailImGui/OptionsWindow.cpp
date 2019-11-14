#include "pch.h"
#include "OptionsWindow.h"
#include "Sail/Application.h"
#include "Sail/KeyBinds.h"
#include "Sail/utils/SailImGui/SailImGui.h"

OptionsWindow::OptionsWindow(bool showWindow) {
	m_app = Application::getInstance();
	m_settings = &m_app->getSettings();
}

OptionsWindow::~OptionsWindow() {}

void OptionsWindow::renderWindow() {
	// Rendering a pause window in the middle of the game window.
	static ImVec2 size = ImVec2(500.f, 600.f);
	ImGui::SetNextWindowPos(m_position);
	ImGui::SetNextWindowSize(size);

	auto& stat = m_settings->applicationSettingsStatic;
	auto& dynamic = m_settings->applicationSettingsDynamic;

	static float x[4] = { 0,0,0,0 };


	if (ImGui::Begin("##Pause Menu", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
		ImGui::DragFloat2("##SIZE", &size[0], 1.0f);
		ImGui::DragFloat4("##POS", &x[0], 1.0f);

		SettingStorage::Setting* sopt = nullptr;
		SettingStorage::DynamicSetting* dopt = nullptr;
		unsigned int selected = 0;
		std::string valueName = "";
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		// GRAPHICS
		ImGui::Text("graphics");
		ImGui::Separator();

		sopt = &stat["graphics"]["fullscreen"];
		selected = sopt->selected;
		if(SailImGui::TextButton(std::string("<##fullscreen").c_str())){
			sopt->setSelected(selected - 1);
		}
		ImGui::SameLine(x[0]);
		valueName = sopt->getSelected().name;
		ImGui::Text(valueName.c_str());
		ImGui::SameLine(x[1]);
		if (SailImGui::TextButton(std::string(">##fullscreen").c_str())) {
			sopt->setSelected(selected + 1);
		}
		ImGui::SameLine(x[2]);
		ImGui::Text("fullscreen");



		sopt = &stat["graphics"]["bloom"];
		selected = sopt->selected;
		if (SailImGui::TextButton(std::string("<##bloom").c_str())) {
			sopt->setSelected(selected - 1);
		}
		ImGui::SameLine(x[0]);
		valueName = sopt->getSelected().name;
		ImGui::Text(valueName.c_str());
		ImGui::SameLine(x[1]);
		if (SailImGui::TextButton(std::string(">##bloom").c_str())) {
			sopt->setSelected(selected + 1);
		}
		ImGui::SameLine(x[2]);
		ImGui::Text("bloom");



		sopt = &stat["graphics"]["fxaa"];
		selected = sopt->selected;
		if (SailImGui::TextButton(std::string("<##fxaa").c_str())) {
			sopt->setSelected(selected - 1);
		}
		ImGui::SameLine(x[0]);
		valueName = sopt->getSelected().name;
		ImGui::Text(valueName.c_str());
		ImGui::SameLine(x[1]);
		if (SailImGui::TextButton(std::string(">##fxaa").c_str())) {
			sopt->setSelected(selected + 1);
		}
		ImGui::SameLine(x[2]);
		ImGui::Text("FXAA");



		sopt = &stat["graphics"]["shadows"];
		selected = sopt->selected;
		if (SailImGui::TextButton(std::string("<##shadows").c_str())) {
			sopt->setSelected(selected - 1);
		}
		ImGui::SameLine(x[0]);
		valueName = sopt->getSelected().name;
		ImGui::Text(valueName.c_str());
		ImGui::SameLine(x[1]);
		if (SailImGui::TextButton(std::string(">##shadows").c_str())) {
			sopt->setSelected(selected + 1);
		}
		ImGui::SameLine(x[2]);
		ImGui::Text("shadows");




		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		// SOUND
		ImGui::Text("Sound");
		ImGui::Separator();

		dopt = &dynamic["sound"]["global"];
		ImGui::SliderFloat(std::string("##Global").c_str(), &dopt->value, dopt->minVal, dopt->maxVal);
		ImGui::SameLine();
		ImGui::Text("global");


		dopt = &dynamic["sound"]["music"];
		ImGui::SliderFloat(std::string("##music").c_str(), &dopt->value, dopt->minVal, dopt->maxVal);
		ImGui::SameLine();
		ImGui::Text("music");

		dopt = &dynamic["sound"]["effects"];
		ImGui::SliderFloat(std::string("##effects").c_str(), &dopt->value, dopt->minVal, dopt->maxVal);
		ImGui::SameLine();
		ImGui::Text("effects");

		dopt = &dynamic["sound"]["voices"];
		ImGui::SliderFloat(std::string("##voices").c_str(), &dopt->value, dopt->minVal, dopt->maxVal);
		ImGui::SameLine();
		ImGui::Text("voices");

		

	}
	ImGui::End();
}