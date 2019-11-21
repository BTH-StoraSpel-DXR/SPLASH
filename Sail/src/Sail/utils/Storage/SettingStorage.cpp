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
		return glm::vec3(
			gameSettingsDynamic["Color" + std::to_string(team)]["r"].value,
			gameSettingsDynamic["Color" + std::to_string(team)]["g"].value,
			gameSettingsDynamic["Color" + std::to_string(team)]["b"].value
		);
	}
	else {
		return glm::vec3(1, 1, 1);
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
	applicationSettingsStatic["graphics"] = std::unordered_map<std::string, Setting>();
	applicationSettingsStatic["graphics"]["fullscreen"] = Setting(1, std::vector<Setting::Option>({
		{ "on", 1.0f }, 
		{ "off",0.0f } 
	}));
	applicationSettingsStatic["graphics"]["fxaa"] = Setting(1, std::vector<Setting::Option>({
		{ "off", 0.0f },
		{ "on", 1.0f },
		}));
	applicationSettingsStatic["graphics"]["bloom"] = Setting(1, std::vector<Setting::Option>({
		{ "off", 0.0f },
		{ "on", 0.2f },
		{ "all the bloom", 1.0f },
	}));
	applicationSettingsStatic["graphics"]["shadows"] = Setting(0, std::vector<Setting::Option>({
		{ "hard",0.0f },
		{ "soft",1.0f }
	}));
	applicationSettingsStatic["graphics"]["water simulation"] = Setting(1, std::vector<Setting::Option>({
		{ "off", 0.0f },
		{ "on", 1.0f }
	}));
}
void SettingStorage::createApplicationDefaultSound() {
	applicationSettingsDynamic["sound"] = std::unordered_map<std::string, DynamicSetting>();
	applicationSettingsDynamic["sound"]["global"]  = DynamicSetting(1.0f, 0.0f, 1.0f);
	applicationSettingsDynamic["sound"]["music"]   = DynamicSetting(1.0f, 0.0f, 1.0f);
	applicationSettingsDynamic["sound"]["effects"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	applicationSettingsDynamic["sound"]["voices"]  = DynamicSetting(1.0f, 0.0f, 1.0f);
}
void SettingStorage::createApplicationDefaultMisc() {
	applicationSettingsStatic["misc"] = std::unordered_map<std::string, Setting>();
	applicationSettingsStatic["Crosshair"] = std::unordered_map<std::string, Setting>();
	auto& crosshairSettings = applicationSettingsDynamic["Crosshair"];
	crosshairSettings["Thickness"] = DynamicSetting(5.0f, 0.0f, 100.0f);
	crosshairSettings["CenterPadding"] = DynamicSetting(10.0f, 0.0f, 20.0f);
	crosshairSettings["Size"] = DynamicSetting(50.0f, 0.0f, 300.0f);
	crosshairSettings["Color R"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	crosshairSettings["Color G"] = DynamicSetting(0.0f, 0.0f, 1.0f);
	crosshairSettings["Color B"] = DynamicSetting(0.0f, 0.0f, 1.0f);
	crosshairSettings["Color A"] = DynamicSetting(1.0f, 0.0f, 1.0f);
}

void SettingStorage::createGameDefaultStructure() {
	createGameDefaultMap();
	createGameModeDefault();
	createGameColorsDefault();
}

void SettingStorage::createGameDefaultMap() {	
	gameSettingsDynamic["map"] = std::unordered_map<std::string, DynamicSetting>();
	gameSettingsDynamic["map"]["sizeX"] =   DynamicSetting(6.0f,	2.0f,	30.0f);
	gameSettingsDynamic["map"]["sizeY"] =   DynamicSetting(6.0f,	2.0f,	30.0f);
	gameSettingsDynamic["map"]["tileSize"] =	DynamicSetting(7.0f, 1.0f, 30.0f);
	gameSettingsDynamic["map"]["clutter"] = DynamicSetting(0.85f,	0.0f,	5.0f);
	gameSettingsDynamic["map"]["seed"] =    DynamicSetting(0.0f,	0.0f,	1000000.0f);
	gameSettingsDynamic["map"]["seed"].value = Utils::rnd() * gameSettingsDynamic["map"]["seed"].maxVal;//Randomize seed on start
	gameSettingsDynamic["map"]["keepSeed"] = DynamicSetting(0.0f, 0.0f, 1.0f);
	gameSettingsDynamic["map"]["sprinklerTime"] = DynamicSetting(60.0f, 0.0f, 600.0f);
	gameSettingsDynamic["map"]["sprinklerIncrement"] = DynamicSetting(10.0f, 5.0f, 300.0f);

	gameSettingsStatic["map"] = std::unordered_map<std::string, Setting>();
	gameSettingsStatic["map"]["sprinkler"] = Setting(0, std::vector<Setting::Option>({
	{ "on", 0.0f },
	{ "off",1.0f }
		}));
	
}

void SettingStorage::createGameModeDefault() {

	gameSettingsStatic["gamemode"] = std::unordered_map<std::string, Setting>();
	gameSettingsStatic["gamemode"]["types"] = Setting(0, std::vector<Setting::Option>({
		{ "Deathmatch", 0.0f },
		{ "Teamdeathmatch", 1.0f },
	}));
	gameSettingsStatic["Teams"]["Deathmatch"] = Setting(1, std::vector<Setting::Option>({
		{ "Spectator", -1.0f },
		{ "Alone", 0.0f },
	}));
	gameSettingsStatic["Teams"]["Teamdeathmatch"] = Setting(1, std::vector<Setting::Option>({
		{ "Spectator", -1.0f },
		{ "Team1", 0.0f },
		{ "Team2", 1.0f },
	}));


}

void SettingStorage::createGameColorsDefault() {
	//gameSettingsDynamic["teamColor"] = std::unordered_map<std::string, DynamicSetting>()
	//Spectator color
	gameSettingsDynamic["Color" + std::to_string(-1)]["r"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	gameSettingsDynamic["Color" + std::to_string(-1)]["g"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	gameSettingsDynamic["Color" + std::to_string(-1)]["b"] = DynamicSetting(1.0f, 0.0f, 1.0f);
	gameSettingsDynamic["Color" + std::to_string(-1)]["a"] = DynamicSetting(1.0f, 0.0f, 1.0f);
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
		gameSettingsDynamic["Color" + std::to_string(i)]["r"] = DynamicSetting(col[i].r / 255.0f, 0.0f, 1.0f);
		gameSettingsDynamic["Color" + std::to_string(i)]["g"] = DynamicSetting(col[i].g / 255.0f, 0.0f, 1.0f);
		gameSettingsDynamic["Color" + std::to_string(i)]["b"] = DynamicSetting(col[i].b / 255.0f, 0.0f, 1.0f);
		gameSettingsDynamic["Color" + std::to_string(i)]["a"] = DynamicSetting(1, 0.0f, 1.0f);
	}

	//Spectators
	gameSettingsStatic["team" + std::to_string(-1)]["color"] = Setting(0, std::vector<Setting::Option>({
			{ "-1", -1.0f },
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
			{ "Dark Green", 10.0f },
			{ "Light Green", 11.0f },
		}));
	}
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
