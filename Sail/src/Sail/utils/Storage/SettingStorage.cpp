#include "pch.h"
#include "SettingStorage.h"
#include "../Utils.h"
#include "..//Regex/Regex.h"

#pragma region OTIONSSTORAGE
SettingStorage::SettingStorage(const std::string& file) {
	createApplicationDefaultStructure();
	if (!loadFromFile(file)) {
		assert(0);
	}
	if (!saveToFile(file)) {
		assert(0);
	}
	createGameDefaultStructure();
	setMap(0, -1, 0);
}

SettingStorage::~SettingStorage() {
	

}

bool SettingStorage::loadFromFile(const std::string& filename) {
	std::string file = Utils::readFile(filename);
	return deSerialize(file, applicationSettingsStatic, applicationSettingsDynamic);
}

bool SettingStorage::saveToFile(const std::string& filename) {
	std::string output = serialize(applicationSettingsStatic, applicationSettingsDynamic);
	Utils::writeFileTrunc(filename, output);
 	return true;
}

std::string SettingStorage::serialize(const std::unordered_map<std::string, std::unordered_map<std::string, Setting>>& stat, const std::unordered_map<std::string, std::unordered_map<std::string, DynamicSetting>>& dynamic) {
	std::string output = "";
	//print content of static settings map
	for (auto const& [areaKey, setting] : stat) {
		output += "#" + areaKey + "\n";
		for (auto const& [settingKey, option] : setting) {
			output += settingKey + "=" + std::to_string(option.selected) + "\n";
		}
		output += "\n";
	}
	//print content of dynamic settings map
	for (auto const& [areaKey, setting] : dynamic) {
		output += "#" + areaKey + "\n";
		for (auto const& [settingKey, option] : setting) {
			output += settingKey + "=" + std::to_string(option.value) + "\n";
		}
		output += "\n";
	}
	return output;
}

bool SettingStorage::deSerialize(const std::string& content, std::unordered_map<std::string, std::unordered_map<std::string, Setting>>& stat, std::unordered_map<std::string, std::unordered_map<std::string, DynamicSetting>>& dynamic) {
	std::string file = content;
	std::string currentArea = "";
	while (file != "") {
		// get a new line from the buffer
		int newline = file.find("\n");
		std::string line = "";
		if (newline != std::string::npos) {
			line = file.substr(0, newline);
			file = file.substr(newline + 1, std::string::npos);
		}
		else {
			line = file;
			file = "";
		}

		// Start new section
		if (line[0] == '#') {

			currentArea = line.substr(1, std::string::npos);
		}
		// import new value
		else if (line != "" && currentArea != "") {
			if (Reg::SettingStatic.match(line.c_str()) == line.size() || Reg::SettingDynamic.match(line.c_str()) == line.size()) {
				int divider = line.find("=");
				std::string name = line.substr(0, divider);
				std::string temp = line.substr(divider + 1, std::string::npos);
				// if found in settings change value
				if (stat.find(currentArea) != stat.end()) {
					if (stat[currentArea].find(name) != stat[currentArea].end()) {
						int selection = std::stoi(temp);
						stat[currentArea][name].setSelected((unsigned int)selection);
					}
				}
				if (dynamic.find(currentArea) != dynamic.end()) {
					if (dynamic[currentArea].find(name) != dynamic[currentArea].end()) {
						float value = std::stof(temp);
						dynamic[currentArea][name].value = value;
					}
				}
			}
		}
	}
	return true;
}

const int SettingStorage::teamColorIndex(const int team) {
	if (team < 12 && team >= -1) {
		std::string name = "team" + std::to_string(team);
		unsigned int value = (unsigned int)(int)gameSettingsStatic["team" + std::to_string(team)]["color"].getSelected().value;
		return (unsigned int)(int)gameSettingsStatic["team" + std::to_string(team)]["color"].getSelected().value;
	}
	else {
		return 0;
	}
}

glm::vec3 SettingStorage::getColor(const int team) {
	if (team < 12 && team >= -1) {
		auto& gameSettingsD = gameSettingsDynamic["Color" + std::to_string(team)];

		return glm::vec3(
			gameSettingsD["r"].value,
			gameSettingsD["g"].value,
			gameSettingsD["b"].value
		);
	}
	else {
		return glm::vec3(1, 1, 1);
	}
}

void SettingStorage::setMap(const int mode, const int index, const int playerCount) {
	srand(time(0));
	switch (mode) {
		//DEATHMATCH
		case 0:
			switch (index) {
			case -1:	setMapValues(5,5,100,2); break;
			case 0:		setMapValues(6,		6,		0.85f,		12397);		break;
			case 1:		setMapValues(10,	10,		0.85f,		34590);		break;
			case 2:		setMapValues(7,		7,		0.85f,		345912);	break;
			case 3:		setMapValues(12,	12,		0.85f,		20);		break;
			case 4:		setMapValues(5,		12,		0.85f,		234923);	break;
			case 5:		setMapValues(3,		6,		0.85f,		100);		break;
			case 6:		setMapValues(6,		10,		0.85f,		300);		break;
			case 7:		setMapValues(7,		8,		0.85f,		123897);	break;
			case 8:		setMapValues(12,	9,		0.85f,		123612);	break;
			case 9:		setMapValues(2,		2,		0.85f,		500);		break;
			case 10:	setMapValues(6,		6,		0.85f,		1200);		break;
			case 11:	setMapValues(4,		4,		0.85f,		2);			break;
			default: break;
				
			}
		break;
		case 1:
			//TEAMDEATHMATCH
			switch (index) {
			case -1:	setMapValues(int(Utils::fastrand() * 30), int(Utils::fastrand() * 30), Utils::fastrand(), int(Utils::fastrand() * 300000)); break;
			case 0:		setMapValues(6, 6, 0.85f, 12397);		break;
			case 1:		setMapValues(10, 10, 0.85f, 34590);		break;
			case 2:		setMapValues(7, 7, 0.85f, 345912);	break;
			case 3:		setMapValues(12, 12, 0.85f, 20);		break;
			case 4:		setMapValues(5, 12, 0.85f, 234923);	break;
			case 5:		setMapValues(3, 6, 0.85f, 100);		break;
			case 6:		setMapValues(6, 10, 0.85f, 300);		break;
			case 7:		setMapValues(7, 8, 0.85f, 123897);	break;
			case 8:		setMapValues(12, 9, 0.85f, 123612);	break;
			case 9:		setMapValues(2, 2, 0.85f, 500);		break;
			case 10:	setMapValues(6, 6, 0.85f, 1200);		break;
			case 11:	setMapValues(4, 4, 0.85f, 2);			break;
			default: break;
				
			}

			
		break;
		default: break;
		
	}








}

SettingStorage::WantedType SettingStorage::matchType(const std::string& value) {
	if (Reg::Number.match(value.c_str()) == value.size()) {
		return WantedType::INT;
	} 
	else if (Reg::DecimalNumber.match(value.c_str()) == value.size()) {
		return WantedType::FLOAT;
	}
	else {
		return WantedType::BOOL;
	}
}

void SettingStorage::createApplicationDefaultStructure() {
	createApplicationDefaultGraphics();
	createApplicationDefaultSound();
	createApplicationDefaultMisc();
}

void SettingStorage::createApplicationDefaultGraphics() {
	auto& applicationSettingsS = applicationSettingsStatic["graphics"] = std::unordered_map<std::string, Setting>();
	applicationSettingsS["fullscreen"] = Setting(1, std::vector<Setting::Option>({
		{ "on", 1.0f }, 
		{ "off",0.0f } 
	}));
	applicationSettingsS["fxaa"] = Setting(1, std::vector<Setting::Option>({
		{ "off", 0.0f },
		{ "on", 1.0f },
		}));
	applicationSettingsS["bloom"] = Setting(1, std::vector<Setting::Option>({
		{ "off", 0.0f },
		{ "on", 0.2f },
		{ "all the bloom", 1.0f },
	}));
	applicationSettingsS["shadows"] = Setting(0, std::vector<Setting::Option>({
		{ "hard",0.0f },
		{ "soft",1.0f }
	}));
	applicationSettingsS["watersimulation"] = Setting(1, std::vector<Setting::Option>({
		{ "off", 0.0f },
		{ "on", 1.0f }
	}));
}
void SettingStorage::createApplicationDefaultSound() {
	auto& applicationSettingsD = applicationSettingsDynamic["sound"] = std::unordered_map<std::string, DynamicSetting>();
	applicationSettingsD["global"]  = DynamicSetting(1.0f, 0.0f, 1.0f);
	applicationSettingsD["music"]   = DynamicSetting(1.0f, 0.0f, 1.0f);
	applicationSettingsD["effects"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	applicationSettingsD["voices"]  = DynamicSetting(1.0f, 0.0f, 1.0f);
}
void SettingStorage::createApplicationDefaultMisc() {
	applicationSettingsStatic["misc"] = std::unordered_map<std::string, Setting>();
	applicationSettingsStatic["Crosshair"] = std::unordered_map<std::string, Setting>();
	auto& crosshairSettings = applicationSettingsDynamic["Crosshair"];
	crosshairSettings["Thickness"] = DynamicSetting(5.0f, 0.0f, 100.0f);
	crosshairSettings["CenterPadding"] = DynamicSetting(10.0f, 0.0f, 20.0f);
	crosshairSettings["Size"] = DynamicSetting(50.0f, 0.0f, 300.0f);
	crosshairSettings["ColorR"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	crosshairSettings["ColorG"] = DynamicSetting(0.0f, 0.0f, 1.0f);
	crosshairSettings["ColorB"] = DynamicSetting(0.0f, 0.0f, 1.0f);
	crosshairSettings["ColorA"] = DynamicSetting(1.0f, 0.0f, 1.0f);
}

void SettingStorage::createGameDefaultStructure() {
	createGameDefaultMap();
	createGameModeDefault();
	createGameColorsDefault();
}

void SettingStorage::createGameDefaultMap() {


	auto& gameSettingD = gameSettingsDynamic["map"] = std::unordered_map<std::string, DynamicSetting>();
	gameSettingD["sizeX"] =   DynamicSetting(6.0f,	2.0f,	30.0f);
	gameSettingD["sizeY"] =   DynamicSetting(6.0f,	2.0f,	30.0f);
	gameSettingD["tileSize"] =	DynamicSetting(7.0f, 1.0f, 30.0f);
	gameSettingD["clutter"] = DynamicSetting(0.85f,	0.0f,	1.0f);
	gameSettingD["seed"] =    DynamicSetting(0.0f,	0.0f,	1000000.0f);
	gameSettingD["sprinklerTime"] = DynamicSetting(75.0f, 0.0f, 600.0f);
	gameSettingD["sprinklerIncrement"] = DynamicSetting(30.0f, 5.0f, 300.0f);

	gameSettingsStatic["map"] = std::unordered_map<std::string, Setting>();
	gameSettingsStatic["map"]["sprinkler"] = Setting(1, std::vector<Setting::Option>({
		{ "on", 0.0f },
		{ "off",1.0f }
	}));




	defaultMaps["Deathmatch"] = Setting(0, std::vector<Setting::Option>({
		{"Random",		-1},
		{"Berlin",		0},
		{"Hamburg",		1},
		{"Stuffgart",	2},
		{"Munich",		3},
		{"Nurburg",		4}, // Fred
		{"Cologne",		5},
		{"Frankfurt",	6},
		{"Bremen",		7},
		{"Flensburg",			8},
		{"Hanover",			9},
		{"Wasserburg",	10}, // Alex - Hanslin/Hansburg/
		{"Lubeck",		11}, // Fred - /
	}));

	defaultMaps["Teamdeathmatch"] = Setting(0, std::vector<Setting::Option>({
		{"Random",		-1},
		{"x",			0},
		{"x",			1},
		{"x",			2},
		{"x",			3},
		{"x",			4}, 
		{"x",			5},
		{"x",			6},
		{"x",			7},
		{"x",			8},
		{"x",			9},
		{"x",			10},
		{"x",			11},
	}));










	
}

void SettingStorage::createGameModeDefault() {

	auto& gameSettingsSGameMode = gameSettingsStatic["gamemode"] = std::unordered_map<std::string, Setting>();
	auto& gameSettingsSTeams = gameSettingsStatic["Teams"] = std::unordered_map<std::string, Setting>();

	gameSettingsSGameMode["types"] = Setting(0, std::vector<Setting::Option>({
		{ "Deathmatch", 0.0f },
		{ "Teamdeathmatch", 1.0f },
	}));
	gameSettingsSTeams["Deathmatch"] = Setting(1, std::vector<Setting::Option>({
		{ "Spectator", -1.0f },
		{ "Alone", 0.0f },
	}));
	gameSettingsSTeams["Teamdeathmatch"] = Setting(1, std::vector<Setting::Option>({
		{ "Spectator", -1.0f },
		{ "Team1", 0.0f },
		{ "Team2", 1.0f },
	}));


}

void SettingStorage::createGameColorsDefault() {
	//gameSettingsDynamic["teamColor"] = std::unordered_map<std::string, DynamicSetting>()
	//Spectator color
	auto& gameSettingsD = gameSettingsDynamic["Color" + std::to_string(-1)];

	gameSettingsD["r"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	gameSettingsD["g"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	gameSettingsD["b"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	gameSettingsD["a"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	//player colors

	std::vector<glm::vec3> col({
		{183,23,33}, // red
		{0,68,253}, // Blue
		{21,131,0}, // Green
		{253,222,45}, // Yellow
		{37,172,238}, // Teal
		{83,3,130}, // Purple
		{255,142,20}, // Orange
		{234,95,176}, // Pink
		{33,4,193}, // Violet
		{82,83,147}, // light grey
		{14,99,68}, // Dark Green
		{148,254,143}, // light green
	});



	for (unsigned int i = 0; i < 12; i++) {
		float f = (i / 12.0f) * glm::two_pi<float>();
		glm::vec4 color(abs(cos(f * 2)), 1 - abs(cos(f * 1.4)), abs(sin(f * 1.1f)), 1.0f);

		auto& gameSettingsD = gameSettingsDynamic["Color" + std::to_string(i)];

		gameSettingsD["r"] = DynamicSetting(col[i].r / 255.0f, 0.0f, 1.0f);
		gameSettingsD["g"] = DynamicSetting(col[i].g / 255.0f, 0.0f, 1.0f);
		gameSettingsD["b"] = DynamicSetting(col[i].b / 255.0f, 0.0f, 1.0f);
		gameSettingsD["a"] = DynamicSetting(1, 0.0f, 1.0f);
	}

	//Spectators
	gameSettingsStatic["team" + std::to_string(-1)]["color"] = Setting(0, std::vector<Setting::Option>({
			{ "White", -1.0f },
		}));
	//Player teams
	for (int i = 0; i < 12; i++) {
		gameSettingsStatic["team" + std::to_string(i)]["color"] = Setting(i, std::vector<Setting::Option>({
			{ "Red", 0.0f },
			{ "Blue", 1.0f },
			{ "Green", 2.0f },
			{ "Yellow", 3.0f },
			{ "Teal", 4.0f },
			{ "Purple", 5.0f },
			{ "Orange", 6.0f },
			{ "Pink", 7.0f },
			{ "Violet", 8.0f },
			{ "Grey", 9.0f },
			{ "D Green", 10.0f },
			{ "L Green", 11.0f },
		}));
	}
}

void SettingStorage::setMapValues(const int x, const int y, const float clutter, const int seed) {
	gameSettingsDynamic["map"]["sizeX"].setValue(x);
	gameSettingsDynamic["map"]["sizeY"].setValue(y);
	gameSettingsDynamic["map"]["clutter"].setValue(clutter);
	gameSettingsDynamic["map"]["seed"].setValue(seed);
}



#pragma endregion








#pragma region OPTION
SettingStorage::Setting::Setting() {
	selected = 0;
}
SettingStorage::Setting::Setting(const unsigned int selectedOption, std::vector<Setting::Option>& asd) {
	selected = selectedOption;
	options = asd;
}

SettingStorage::Setting::~Setting() {
}

void SettingStorage::Setting::setSelected(const unsigned int selection) {
	selected = selection;
	if (selected == options.size()) {
		selected = 0;
	}
	if (selected == -1) {
		selected = options.size() - 1;
	}
	if (selected > options.size()) {
		selected = options.size() - 1;
	}
}

const SettingStorage::Setting::Option& SettingStorage::Setting::getSelected() {
	return options[selected];
}

#pragma endregion
