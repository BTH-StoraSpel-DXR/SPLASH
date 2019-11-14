#pragma once

#include "SailImGuiWindow.h"
#include "Sail/Application.h"

class OptionsWindow : public SailImGuiWindow {

public:
	OptionsWindow(bool showWindow = true);
	~OptionsWindow();

	virtual void renderWindow() override;

private:
	Application* m_app;
	SettingStorage* m_settings;
};