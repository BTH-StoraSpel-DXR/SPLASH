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
	
	auto& stat = m_settings->applicationSettingsStatic;
	auto& dynamic = m_settings->applicationSettingsDynamic;

	static float x[4] = { 
		ImGui::GetWindowContentRegionWidth()*0.5f,
		0,
		ImGui::GetWindowContentRegionWidth() * 0.95f,
		0 
	};
#ifdef DEVELOPMENT
	ImGui::DragFloat4("##POS", &x[0], 1.0f);
#endif
	SettingStorage::Setting* sopt = nullptr;
	SettingStorage::DynamicSetting* dopt = nullptr;
	unsigned int selected = 0;
	std::string valueName = "";
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	// GRAPHICS
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	ImGui::Text("graphics");
	ImGui::PopStyleColor();
	ImGui::Separator();


	static std::vector<std::string> options = { "fullscreen","bloom","shadows","fxaa","water simulation" };
	for (auto & optionName : options) {
		sopt = &stat["graphics"][optionName];
		selected = sopt->selected;
		ImGui::Text(optionName.c_str());
		ImGui::SameLine(x[0]);
		if (SailImGui::TextButton(std::string("<##"+ optionName).c_str())) {
			sopt->setSelected(selected - 1);
		}
		ImGui::SameLine(x[1]);
		valueName = sopt->getSelected().name;
		SailImGui::cText(valueName.c_str(), x[2]);
		ImGui::SameLine(x[2]);
		if (SailImGui::TextButton(std::string(">##"+ optionName).c_str())) {
			sopt->setSelected(selected + 1);
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	// SOUND
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	ImGui::Text("Sound");
	ImGui::PopStyleColor();
	ImGui::Separator();

	const static std::vector<std::string> soundSettings = { "global", "music", "effects", "voices" };
	for (auto& settingName : soundSettings) {
		dopt = &dynamic["sound"][settingName];
		ImGui::Text(settingName.c_str());
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
		float val = dopt->value * 100.0f;
		if (ImGui::SliderFloat(std::string("##"+settingName).c_str(), &val, dopt->minVal, dopt->maxVal * 100.0f, "%.1f%%")) {
			dopt->setValue(val * 0.01f);
		}
	}	
}

bool OptionsWindow::renderGameOptions() {

	static float x[4] = {
	ImGui::GetWindowContentRegionWidth() * 0.5f,
	0,
	ImGui::GetWindowContentRegionWidth() * 0.95f,
	0
	};
#ifdef DEVELOPMENT
	ImGui::DragFloat4("##POS", &x[0], 1.0f);
#endif
	bool settingsChanged = false;
	static auto& dynamic = m_app->getSettings().gameSettingsDynamic;
	static auto& stat = m_app->getSettings().gameSettingsStatic;
	
	SailImGui::HeaderText("Map Settings");
	ImGui::Separator();

	SettingStorage::DynamicSetting* mapSizeX = &m_app->getSettings().gameSettingsDynamic["map"]["sizeX"];
	SettingStorage::DynamicSetting* mapSizeY = &m_app->getSettings().gameSettingsDynamic["map"]["sizeY"];

	static int size[] = { 0,0 };
	size[0] = (int)mapSizeX->value;
	size[1] = (int)mapSizeY->value;
	ImGui::Text("MapSize"); 
	ImGui::SameLine(x[0]);
	ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
	if (ImGui::SliderInt2("##MapSizeXY", size, (int)mapSizeX->minVal, (int)mapSizeX->maxVal)) {
		mapSizeX->value = size[0];
		mapSizeY->value = size[1];
		settingsChanged = true;
	}

	int seed = dynamic["map"]["seed"].value;
	ImGui::Text("Seed"); 
	ImGui::SameLine(x[0]);
	ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
	if (ImGui::InputInt("##SEED", &seed)) {
		dynamic["map"]["seed"].setValue(seed);
		settingsChanged = true;
	}
	ImGui::Text("Clutter"); 
	ImGui::SameLine(x[0]);
	ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
	float val = m_app->getSettings().gameSettingsDynamic["map"]["clutter"].value * 100;
	if (ImGui::SliderFloat("##Clutter",
		&val,
		m_app->getSettings().gameSettingsDynamic["map"]["clutter"].minVal * 100,
		m_app->getSettings().gameSettingsDynamic["map"]["clutter"].maxVal * 100,
		"%.1f%%"
	)) {
		m_app->getSettings().gameSettingsDynamic["map"]["clutter"].setValue(val * 0.01f);
		settingsChanged = true;
	}

	SailImGui::HeaderText("Gamemode Settings");
	ImGui::Separator();

	SettingStorage::Setting* sopt = nullptr;
	SettingStorage::DynamicSetting* dopt = nullptr;
	unsigned int selected = 0;
	std::string valueName = "";


	static std::vector<std::string> options = { "types"};
	for (auto& optionName : options) {
		sopt = &stat["gamemode"][optionName];
		selected = sopt->selected;
		ImGui::Text(optionName.c_str());
		ImGui::SameLine(x[0]);
		if (SailImGui::TextButton(std::string("<##" + optionName).c_str())) {
			sopt->setSelected(selected - 1);
		}
		ImGui::SameLine(x[1]);
		valueName = sopt->getSelected().name;
		SailImGui::cText(valueName.c_str(), x[2]);
		ImGui::SameLine(x[2]);
		if (SailImGui::TextButton(std::string(">##" + optionName).c_str())) {
			sopt->setSelected(selected + 1);
		}
	}

	return settingsChanged;
}
