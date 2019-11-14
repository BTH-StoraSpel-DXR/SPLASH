#pragma once
#include "imgui.h"

class SailImGuiWindow {
public:
	SailImGuiWindow();
	SailImGuiWindow(bool showWindow);
	virtual ~SailImGuiWindow();

	bool isWindowOpen();
	virtual void toggleWindow();
	virtual void showWindow(bool show);
	void setPosition(const ImVec2& pos);

	virtual void renderWindow() = 0;

	static float EaseInOut(float time, float startValue, float endValue, float duration);
	static float EaseIn(float time, float startValue, float endValue, float duration);
	static float EaseOut(float time, float startValue, float endValue, float duration);
protected:
	bool m_isWindowOpen;
	ImVec2 m_position;

private:
};