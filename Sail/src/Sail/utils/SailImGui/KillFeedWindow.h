#pragma once

#include "SailImGuiWindow.h"

#include <string>
#include <vector>

class GameDataTracker;

class KillFeedWindow : public SailImGuiWindow {
public:
	KillFeedWindow(bool showWindow = true);
	~KillFeedWindow();

	virtual void renderWindow() override;
	void updateTiming(float dt);

private:
	bool m_doRender;
	GameDataTracker& m_gameDataTracker;
	float m_maxTimeShowed;
	std::vector<std::pair<float, std::string>> m_kills;
};