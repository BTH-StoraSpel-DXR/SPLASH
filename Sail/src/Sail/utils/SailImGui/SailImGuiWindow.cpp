#include "pch.h"
#include "SailImGuiWindow.h"

SailImGuiWindow::SailImGuiWindow()
	: m_isWindowOpen(false)
{ }

SailImGuiWindow::SailImGuiWindow(bool showWindow)
	: m_isWindowOpen(showWindow)
{ }

SailImGuiWindow::~SailImGuiWindow() {
}

bool SailImGuiWindow::isWindowOpen() {
	return m_isWindowOpen;
}

void SailImGuiWindow::toggleWindow() {
	m_isWindowOpen = !m_isWindowOpen;
}

void SailImGuiWindow::showWindow(bool show) {
	m_isWindowOpen = show;
}

void SailImGuiWindow::setPosition(const ImVec2& pos) {
	m_position = pos;
}

float SailImGuiWindow::EaseInOut(float time, float startValue, float endValue, float duration) {
	time /= duration / 2.f;
	if (time < 1.f) return endValue / 2.f * time * time + startValue;
	time--;
	return -endValue / 2.f * (time * (time - 2.f) - 1.f) + startValue;
}
float SailImGuiWindow::EaseIn(float time, float startValue, float endValue, float duration) {
	time /= duration;
	return endValue * time * time + startValue;
}
float SailImGuiWindow::EaseOut(float time, float startValue, float endValue, float duration) {
	time /= duration;
	return -endValue * time * (time - 2.f) + startValue;
}
