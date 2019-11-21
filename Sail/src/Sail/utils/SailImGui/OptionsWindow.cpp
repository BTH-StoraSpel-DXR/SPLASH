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
	
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	// CROSSHAIR
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	ImGui::Text("Crosshair");
	ImGui::PopStyleColor();
	ImGui::Separator();

	const static std::vector<std::string> crossHairSettings = { "Thickness", "CenterPadding", "Size", "Color R", "Color G", "Color B", "Color A" };
	for (auto& settingName : crossHairSettings) {
		dopt = &dynamic["Crosshair"][settingName];
		ImGui::Text(settingName.c_str());
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
		if (ImGui::SliderFloat(std::string("##"+settingName).c_str(), &dopt->value, dopt->minVal, dopt->maxVal, "%.1f")) {
		}
	}


	/* Left out because not fully implemented, make it so if you wish, henry! */
	//Drawxhare
	//if (ImGui::BeginChild("##XHARE", ImVec2(0, 100))) {
	//	
	//	//drawCrosshair();
	//}
	//ImGui::EndChild();
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

	bool keepSeed = (dynamic["map"]["keepSeed"].value > 0);
	ImGui::Text("Keep Seed");
	ImGui::SameLine(x[0]);
	ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
	if (ImGui::Checkbox("##KEEPSEED", &keepSeed)) {
		dynamic["map"]["keepSeed"].setValue(keepSeed ? 1.0f : 0.0f);
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

void OptionsWindow::drawCrosshair() {
	// Fetch current settings from settingsStorage
	auto& stat = m_settings->applicationSettingsStatic;
	auto& dynamic = m_settings->applicationSettingsDynamic;
	float screenWidth = 100.0f;
	float screenHeight = 100.0f;

	screenWidth = ImGui::GetCurrentWindow()->Pos.x + 5;
	screenHeight = ImGui::GetCurrentWindow()->Pos.y + 5;

	auto& crosshairSettings = dynamic["Crosshair"];
	float thickness = crosshairSettings["Thickness"].value;
	float centerPadding = crosshairSettings["CenterPadding"].value;
	float size = crosshairSettings["Size"].value;
	float outerAlteredPadding = crosshairSettings["OuterAlteredPadding"].value;

	// Crosshair
	ImVec2 crosshairSize{
		size,
		size
	};
	ImVec2 center{
		screenWidth * 0.5f,
		screenHeight * 0.5f
	};
	ImVec2 topLeft{
		center.x - crosshairSize.x * 0.5f,
		center.y - crosshairSize.y * 0.5f
	};
	ImVec2 top{
		topLeft.x + crosshairSize.x * 0.5f,
		topLeft.y
	};
	ImVec2 bot{
		center.x,
		center.y + crosshairSize.y * 0.5f
	};
	ImVec2 right{
		center.x + crosshairSize.x * 0.5f,
		center.y
	};
	ImVec2 left{
		center.x - crosshairSize.x * 0.5f,
		center.y
	};


	//ImGui::SetNextWindowPos(topLeft);
	//ImGui::SetNextWindowSize(crosshairSize);

	ImGuiWindowFlags crosshairFlags = ImGuiWindowFlags_NoCollapse;
	crosshairFlags |= ImGuiWindowFlags_NoResize;
	crosshairFlags |= ImGuiWindowFlags_NoMove;
	crosshairFlags |= ImGuiWindowFlags_NoNav;
	crosshairFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	crosshairFlags |= ImGuiWindowFlags_NoTitleBar;
	crosshairFlags |= ImGuiWindowFlags_NoBackground;
//	ImGui::Begin("Crosshair", NULL, crosshairFlags);

	ImVec4 color2{ 1.0f, 0.0f, 0.0f, 1.0f };
	const ImU32 color = ImColor(color2);

	ImVec2 center_padded_top{
		top.x,
		center.y - centerPadding
	};
	ImVec2 center_padded_bot{
		top.x,
		center.y + centerPadding
	};
	ImVec2 center_padded_right{
		center.x + centerPadding,
		right.y
	};
	ImVec2 center_padded_left{
		center.x - centerPadding,
		left.y
	};

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	//		|
	//   
	//
	draw_list->AddLine(
		top,
		center_padded_top,
		color,
		thickness
	);

	//		|
	//   
	//		|
	draw_list->AddLine(
		bot,
		center_padded_bot,
		color,
		thickness
	);

	//		|
	//		    --
	//		|
	draw_list->AddLine(
		right,
		center_padded_right,
		color,
		thickness
	);

	//		|
	//	--	   --
	//		|
	draw_list->AddLine(
		left,
		center_padded_left,
		color,
		thickness
	);



	// Set to True/False by  CrosshairSystem
	ImVec2 topRight{
		right.x,
		top.y
	};
	ImVec2 botRight{
		right.x,
		center.y + size * 0.5f
	};
	ImVec2 botLeft{
		left.x,
		botRight.y
	};

	ImVec2 center_padded_topLeft{
		center.x - centerPadding,
		center.y - centerPadding
	};
	ImVec2 center_padded_topRight{
		center.x + centerPadding,
		center.y - centerPadding
	};
	ImVec2 center_padded_botRight{
		center.x + centerPadding,
		center.y + centerPadding
	};
	ImVec2 center_padded_botLeft{
		center.x - centerPadding,
		center.y + centerPadding
	};

	// Set alpha-value of the color based on how long it has been altered for (F1->0)
	//onHitColor.w = 1 - (c->passedTimeSinceAlteration / c->durationOfAlteredCrosshair);
	const ImU32 onHitcolor = color;//ImColor(onHitColor);

	//	\
	//
	//
	// Draw an additional cross
	draw_list->AddLine(
		topLeft,
		center_padded_topLeft,
		onHitcolor,
		thickness
	);
	//	\	/
	//
	//
	// Draw an additional cross
	draw_list->AddLine(
		topRight,
		center_padded_topRight,
		onHitcolor,
		thickness
	);
	//	\	/
	//
	//		\
	// Draw an additional cross
	draw_list->AddLine(
		botRight,
		center_padded_botRight,
		onHitcolor,
		thickness
	);
	//	\	/
	//
	//	/   \
	// Draw an additional cross
	draw_list->AddLine(
		botLeft,
		center_padded_botLeft,
		onHitcolor,
		thickness
	);
}
