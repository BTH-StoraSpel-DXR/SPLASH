#pragma once
#include <map>

class SettingStorage {
public:
	enum WantedType {
		BOOL,
		INT,
		FLOAT
	};
	class Setting {
	public:
		struct Option {
			std::string name;
			float value;
		};

		Setting();
		Setting(const unsigned int selected, std::vector<Setting::Option>& options);
		~Setting();

		void setSelected(const unsigned int selection);
		const Option& getSelected();
		unsigned int selected;
		std::vector<Option> options;
	};
	class DynamicSetting {
	public:
		DynamicSetting() : DynamicSetting(0.0f, 0.0f, 1.0f) {}
		DynamicSetting(const float& _value, const float& _min, const float& _max) :
			value(_value),
			minVal(_min),
			maxVal(_max) {
		}

		void setValue(const float& _value) {
			value = _value;
			if (value <= minVal) {
				value = minVal;
			}
			if (value >= maxVal) {
				value = maxVal;
			}
		}
		float value;
		float minVal;
		float maxVal;

	};

	SettingStorage(const std::string& filename);
	~SettingStorage();

	bool loadFromFile(const std::string& filename);
	bool saveToFile(const std::string& filename);

	std::string serialize(const std::unordered_map<std::string, std::unordered_map<std::string, Setting>>& stat, const std::unordered_map<std::string, std::unordered_map<std::string, DynamicSetting>>& dynamic);
	bool deSerialize(const std::string& msg, std::unordered_map<std::string, std::unordered_map<std::string, Setting>>& stat, std::unordered_map<std::string, std::unordered_map<std::string, DynamicSetting>>& dynamic);

	const int teamColorIndex(const int team);
	glm::vec3 getColor(const int colorIndex);

	void setMap(const int mode, const int index, const int playerCount);



	// settings[area][setting].selected == current selected setting; 
	// settings[area][setting].options[selected].value == value of selected setting; 

	std::unordered_map<std::string, std::unordered_map<std::string, Setting>> applicationSettingsStatic;
	std::unordered_map<std::string, std::unordered_map<std::string, DynamicSetting>> applicationSettingsDynamic;


	std::unordered_map<std::string, std::unordered_map<std::string, Setting>> gameSettingsStatic;
	std::unordered_map<std::string, Setting> defaultMaps;
	std::unordered_map<std::string, std::unordered_map<std::string, DynamicSetting>> gameSettingsDynamic;


private:

	WantedType matchType(const std::string& value);

private:

	void createApplicationDefaultStructure();
	void createApplicationDefaultGraphics();
	void createApplicationDefaultSound();
	void createApplicationDefaultMisc();

	void createGameDefaultStructure();
	void createGameDefaultMap();
	void createGameModeDefault();
	void createGameColorsDefault();

	void setMapValues(const int x, const int y, const float clutter, const int seed);

};