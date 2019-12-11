#include "pch.h"
#include "OptionsWindow.h"
#include "Sail/Application.h"
#include "Sail/KeyBinds.h"
#include "Sail/utils/SailImGui/SailImGui.h"
#include "Sail/entities/ECS.h"

OptionsWindow::OptionsWindow(bool showWindow) {
	m_app = Application::getInstance();
	m_settings = &m_app->getSettings();
	m_levelSystem = ECS::Instance()->getSystem<LevelSystem>();

	auto& dynamic = m_app->getSettings().gameSettingsDynamic;
	auto& stat = m_app->getSettings().gameSettingsStatic;
	m_levelSystem->destroyWorld();
	m_levelSystem->seed = dynamic["map"]["seed"].value;
	m_levelSystem->clutterModifier = dynamic["map"]["clutter"].value * 100;
	m_levelSystem->xsize = dynamic["map"]["sizeX"].value;
	m_levelSystem->ysize = dynamic["map"]["sizeY"].value;

	m_levelSystem->generateMap();

}

OptionsWindow::~OptionsWindow() {}

void OptionsWindow::renderWindow() {
	// Rendering a pause window in the middle of the game window.
	
	auto& stat = m_settings->applicationSettingsStatic;
	auto& dynamic = m_settings->applicationSettingsDynamic;
	float x[4] = { 
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


	static std::vector<std::string> options = { "fullscreen","bloom","shadows","fxaa","watersimulation" };
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

	const static std::vector<std::string> soundSettings = { "global"/*, "music", "effects", "voices"*/ };
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

	const static std::vector<std::string> crossHairSettings = { "Thickness", "CenterPadding", "Size"};
	for (auto& settingName : crossHairSettings) {
		dopt = &dynamic["Crosshair"][settingName];
		ImGui::Text(settingName.c_str());
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
		if (ImGui::SliderFloat(std::string("##"+settingName).c_str(), &dopt->value, dopt->minVal, dopt->maxVal, "%.1f")) {
		}
	}
	//ImVec4 col (
	//		dynamic["Crosshair"]["ColorR"].value,
	//		dynamic["Crosshair"]["ColorG"].value,
	//		dynamic["Crosshair"]["ColorB"].value,
	//		dynamic["Crosshair"]["ColorA"].value
	//);
	//
	//ImGui::Text("Color");
	//ImGui::SameLine(x[0]);
	////ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - x[0]);
	//ImGui::ColorButton("##ColorPickerasd", col);
	//dynamic["Crosshair"]["ColorR"].setValue(col.x);
	//dynamic["Crosshair"]["ColorG"].setValue(col.y);
	//dynamic["Crosshair"]["ColorB"].setValue(col.z);
	//dynamic["Crosshair"]["ColorA"].setValue(col.z);
	
	float color[4] = {
		dynamic["Crosshair"]["ColorR"].value,
		dynamic["Crosshair"]["ColorG"].value,
		dynamic["Crosshair"]["ColorB"].value,
		dynamic["Crosshair"]["ColorA"].value
	};

	ImGui::Text("Color");
	ImGui::SameLine(x[0]);
	ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - x[0]);
	if (ImGui::ColorPicker4("##ASDAS", color)) {
	
		dynamic["Crosshair"]["ColorR"].setValue(color[0]);
		dynamic["Crosshair"]["ColorG"].setValue(color[1]);
		dynamic["Crosshair"]["ColorB"].setValue(color[2]);
		dynamic["Crosshair"]["ColorA"].setValue(color[3]);
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

	float x[4] = {
	ImGui::GetWindowContentRegionWidth() * 0.5f,
	0,
	ImGui::GetWindowContentRegionWidth() * 0.95f,
	0
	};
#ifdef DEVELOPMENT
	//ImGui::DragFloat4("##POS", &x[0], 1.0f);
#endif
	bool settingsChanged = false;
	bool mapChanged = false;
	static auto& dynamic = m_app->getSettings().gameSettingsDynamic;
	static auto& stat = m_app->getSettings().gameSettingsStatic;
	static auto& defaultMap = m_app->getSettings().defaultMaps;
	
	SailImGui::HeaderText("Map Settings");
	ImGui::Separator();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Columns(2,"##asd",false);
	//ImGui::SetColumnWidth(0, ImGui::GetWindowContentRegionWidth() * 0.5f);
	//ImGui::SetColumnWidth(1, ImGui::GetWindowContentRegionWidth() * 0.5f);
	//ImGui::Text(std::string(std::to_string((int)ImGui::GetCursorPosX()) + ";" + std::to_string((int)ImGui::GetColumnOffset()) +";"+ std::to_string((int)ImGui::GetColumnWidth())).c_str());
	//ImGui::NextColumn();
	//ImGui::Text(std::string(std::to_string((int)ImGui::GetCursorPosX()) + ";" + std::to_string((int)ImGui::GetColumnOffset()) + ";" + std::to_string((int)ImGui::GetColumnWidth())).c_str());
	//ImGui::NextColumn();
	
	ImGui::Text("Map Selection");
	//ImGui::SameLine(x[0]);
	ImGui::NextColumn();
	
	std::string gamemode = stat["gamemode"]["types"].getSelected().name;
	std::string currentMap = defaultMap[gamemode].getSelected().name;
	if (ImGui::BeginCombo("##MAPCOMBO", currentMap.c_str(), ImGuiComboFlags_NoArrowButton)) {

		unsigned int i = 0;
		for (auto& option : defaultMap[gamemode].options) {
			if (ImGui::Selectable(std::string(option.name).c_str(), option.name == currentMap)) {
				defaultMap[gamemode].setSelected(i);
				m_app->getSettings().setMap(stat["gamemode"]["types"].selected, i-1, 0); // TODO: Add playercount
				settingsChanged = true;
				mapChanged = true;
				break;
			}
			i++;
		}
		ImGui::EndCombo();
	}
	
	ImGui::NextColumn();




	SettingStorage::Setting* sopt = nullptr;
	SettingStorage::DynamicSetting* dopt = nullptr;
	unsigned int selected = 0;
	std::string valueName = "";

	static std::vector<std::string> MapOptions = { "sprinkler", "Powerup" };
	for (auto& optionName : MapOptions) {
		sopt = &stat["map"][optionName];
		selected = sopt->selected;
		ImGui::Text(optionName.c_str());
		//ImGui::SameLine(x[0]);
		ImGui::NextColumn();
		if (SailImGui::TextButton(std::string("<##" + optionName).c_str())) {
			sopt->setSelected(selected - 1);
		}
		ImGui::SameLine(1);
		valueName = sopt->getSelected().name;

		SailImGui::cText(valueName.c_str(), ImGui::GetColumnOffset() + ImGui::GetColumnWidth()-20);
		ImGui::SameLine(ImGui::GetColumnWidth()-40);
		if (SailImGui::TextButton(std::string(">##" + optionName).c_str())) {
			sopt->setSelected(selected + 1);
		}
		ImGui::NextColumn();
	}

	ImGui::NextColumn();
	ImGui::Columns(1);
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Columns(2, "##DASDA2", false);
	//ImGui::SetColumnWidth(0, 100);
	ImGui::Text("Preview");
	ImGui::NextColumn();
	//ImGui::Columns(1);
	if (ImGui::BeginChild("##MAPPRINT", ImVec2(0, 150))) {
		drawMap();

	}
	ImGui::EndChild();

	ImGui::Columns(1);

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();


	if (ImGui::CollapsingHeader("Advanced Settings##map")) {
		SettingStorage::DynamicSetting* mapSizeX = &m_app->getSettings().gameSettingsDynamic["map"]["sizeX"];
		SettingStorage::DynamicSetting* mapSizeY = &m_app->getSettings().gameSettingsDynamic["map"]["sizeY"];

		ImGui::Indent();
		static int size[] = { 0,0 };
		size[0] = (int)mapSizeX->value;
		size[1] = (int)mapSizeY->value;
		ImGui::Text("MapSize (x,y)");
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
		if (ImGui::SliderInt2("##MapSizeXY", size, (int)mapSizeX->minVal, (int)mapSizeX->maxVal)) {
			mapSizeX->value = size[0];
			mapSizeY->value = size[1];
			settingsChanged = true;
			mapChanged = true;
		}

		int seed = dynamic["map"]["seed"].value;
		ImGui::Text("Seed");
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f - 86);
		if (ImGui::InputInt("##SEED", &seed,0,0)) {
			dynamic["map"]["seed"].setValue(seed);
			settingsChanged = true;
			mapChanged = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPos().x);
		if (ImGui::Button("Randomize")) {
			dynamic["map"]["seed"].setValue(rand()%300000);
			settingsChanged = true;
			mapChanged = true;
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




		ImGui::Unindent();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	SailImGui::HeaderText("Game Settings");
	ImGui::Separator();


	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);

	static std::vector<std::string> gameOptions = { "types"};
	for (auto& optionName : gameOptions) {
		sopt = &stat["gamemode"][optionName];
		selected = sopt->selected;

		ImGui::Text(optionName.c_str());
		ImGui::SameLine(x[0]);
		if (SailImGui::TextButton(std::string("<##" + optionName).c_str())) {
			sopt->setSelected(selected - 1);
			settingsChanged = true;
			mapChanged = true;
		}
		ImGui::SameLine(x[1]);
		valueName = sopt->getSelected().name;
		SailImGui::cText(valueName.c_str(), x[2]);
		ImGui::SameLine(x[2]);
		if (SailImGui::TextButton(std::string(">##" + optionName).c_str())) {
			sopt->setSelected(selected + 1);
			settingsChanged = true;
			mapChanged = true;
		}

	}
	ImGui::PopItemFlag();
	ImGui::PopStyleColor();

	if (ImGui::CollapsingHeader("Advanced Settings##game")) {
		ImGui::Indent();
		bool powActive = stat["map"]["Powerup"].getSelected().value == 0.0f ? true : false;
		if (!powActive) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		}

		SailImGui::HeaderText("PowerUp ");
		ImGui::Text("Duration");
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
		float val = m_app->getSettings().gameSettingsDynamic["powerup"]["duration"].value;
		if (ImGui::SliderFloat("##Duration",
			&val,
			m_app->getSettings().gameSettingsDynamic["powerup"]["duration"].minVal,
			m_app->getSettings().gameSettingsDynamic["powerup"]["duration"].maxVal,
			"%.0fs"
		)) {
			m_app->getSettings().gameSettingsDynamic["powerup"]["duration"].setValue(val);
			settingsChanged = true;
		}

		ImGui::Text("respawnTime");
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
		val = m_app->getSettings().gameSettingsDynamic["powerup"]["respawnTime"].value;
		if (ImGui::SliderFloat("##respawnTime",
			&val,
			m_app->getSettings().gameSettingsDynamic["powerup"]["respawnTime"].minVal,
			m_app->getSettings().gameSettingsDynamic["powerup"]["respawnTime"].maxVal,
			"%.0fs"
		)) {
			m_app->getSettings().gameSettingsDynamic["powerup"]["respawnTime"].setValue(val);
			settingsChanged = true;
		}
		ImGui::Text("count");
		ImGui::SameLine(x[0]);
		ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5f);
		val = m_app->getSettings().gameSettingsDynamic["powerup"]["count"].value;
		if (ImGui::SliderFloat("##count",
			&val,
			m_app->getSettings().gameSettingsDynamic["powerup"]["count"].minVal,
			m_app->getSettings().gameSettingsDynamic["powerup"]["count"].maxVal,
			"%.0f"
		)) {
			m_app->getSettings().gameSettingsDynamic["powerup"]["count"].setValue(val);
			settingsChanged = true;
		}
		if (!powActive) {
			ImGui::PopItemFlag();
			ImGui::PopStyleColor();
		}

	}

	if (mapChanged) {
		updateMap();
	}


	return settingsChanged;
}

void OptionsWindow::updateMap() {
	
	float oldCount = m_levelSystem->powerUpSpawnPoints.size();
	m_levelSystem->destroyWorld();
	m_levelSystem->seed = m_settings->gameSettingsDynamic["map"]["seed"].value;
	m_levelSystem->clutterModifier = m_settings->gameSettingsDynamic["map"]["clutter"].value * 100;
	m_levelSystem->xsize = m_settings->gameSettingsDynamic["map"]["sizeX"].value;
	m_levelSystem->ysize = m_settings->gameSettingsDynamic["map"]["sizeY"].value;

	m_levelSystem->generateMap(); 
	auto& count = m_settings->gameSettingsDynamic["powerup"]["count"];
	count.maxVal = m_levelSystem->powerUpSpawnPoints.size();
	float p = count.value / oldCount;
	p = std::clamp(p, 0.0f, 1.0f);
	count.value = p * count.maxVal;

}

 jbscsddvdrvoid OptionsWindow::drawCrosshair() {
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

void OptionsWindow::drawMap() {
	int *** tiles = ECS::Instance()->getSystem<LevelSystem>()->getTiles();
	if (!tiles) {
		return;
	}
	int maxX = m_levelSystem->xsize;
	int maxY = m_levelSystem->ysize;
	int rooms = m_levelSystem->numberOfRooms;

	ImVec2 screenSize(
		ImGui::GetWindowContentRegionWidth()-10,
		ImGui::GetWindowHeight() - 10
	);
	float size = 0;
	size = (float)(screenSize.x) / (float)maxX;
	if (size * maxY > screenSize.y) {
		size = (float)(screenSize.y) / (float)maxY;
	}


	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 p = ImGui::GetCursorScreenPos();
	p.x += (ImGui::GetWindowContentRegionWidth()) * 0.5f - (size*maxX*0.5f);
	float ox = p.x, oy = p.y;

	ImU32 red = ImColor(ImVec4(1,0,0,1));
	ImU32 black = ImColor(ImVec4(0, 0, 0, 1));

	for (unsigned int x = 0; x < maxX; x++) {
		for (unsigned int y = 0; y < maxY; y++) {
			//ImVec4 col((float)x / (float)maxX, (float)y / (float)maxY, 1, 1);

			ImVec4 col(
				(float)tiles[x][y][1]/((float)rooms*0.33f),
				1.0f-((float)tiles[x][y][1]/((float)rooms*0.66f)),
				1.0f-(float)tiles[x][y][1]/(float)rooms,
				1
			);
			

			const ImU32 color = ImColor(col);
			draw_list->AddRectFilled(
				ImVec2(ox + (x * size), oy + (y * size)),
				ImVec2(ox + (x * size) + size, oy + (y * size) + size),
				tiles[x][y][1] == 0 ? black : color
			);

			if (tiles[x][y][2] > 0) {
				draw_list->AddLine(
					ImVec2(),
					ImVec2(),
					1,
					(ImU32)ImColor(1.0f, 0.0f, 0.0f, 1.0f)
				);
			}
			
			



		}
		//draw_list->AddLine(
		//	ImVec2(ox + (x * size), oy),
		//	ImVec2(ox + (x * size) + size, oy),
		//	red,
		//	1
		//);
		//draw_list->AddLine(
		//	ImVec2(ox + (x * size), oy + (maxY)*size ),
		//	ImVec2(ox + (x * size) + size, oy + (maxY) * size),
		//	red,
		//	1
		//);
	}
}


