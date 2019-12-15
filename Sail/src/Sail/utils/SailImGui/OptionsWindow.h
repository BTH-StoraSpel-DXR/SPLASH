#pragma once

#include "SailImGuiWindow.h"
#include "Sail/Application.h"
#include "Sail/entities/systems/Gameplay/LevelSystem/LevelSystem.h"


class OptionsWindow : public SailImGuiWindow {

public:
	OptionsWindow(bool showWindow = true);
	~OptionsWindow();

	virtual void renderWindow() override;
	bool renderGameOptions();
	void updateMap();
	void setDisabled(bool b = true);

private:
	bool m_disabled = false;

	Application* m_app = nullptr;
	SettingStorage* m_settings = nullptr;
	LevelSystem* m_levelSystem = nullptr;

	int* m_keyToChange;


private:
	void drawCrosshair();
	void drawMap();
	void resetKeyBind(int key = -1); // -1 to reset all
};