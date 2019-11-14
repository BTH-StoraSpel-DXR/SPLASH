#pragma once
#include <string>
#include <map>
#include "imgui.h"

class ImGuiHandler {
public:
	static ImGuiHandler* Create();

	ImGuiHandler() : m_showMetrics(false) {}
	virtual ~ImGuiHandler() {}

	virtual void init() = 0;

	virtual void begin() = 0;
	virtual void end() = 0;

	void applySailStyle();

	const std::map<std::string, ImFont*>& getFontMap();
	ImFont* getFont(const std::string& font);
	void showMetrics(const bool show);

protected:
	void addFonts();

protected:
	bool m_showMetrics;
	std::map<std::string, ImFont*> m_fonts;
};