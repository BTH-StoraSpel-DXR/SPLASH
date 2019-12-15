#include "pch.h"
#include "ImGuiHandler.h"
#include "imgui.h"

#define USE_ONLY_ONE_FONT

void ImGuiHandler::applySailStyle() {
	auto& style = ImGui::GetStyle();
	style.WindowBorderSize = 1.0f;
	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 3.0f;
	style.FramePadding.y = 0.0f;
	style.PopupRounding = 1.0f;
	style.ScrollbarRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 3.0f;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);


	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 0.7f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.16f, 0.2f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.29f, 0.25f, 0.2f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.51f, 0.28f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.51f, 0.28f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.61f, 0.40f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.51f, 0.28f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.5f, 0.4f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.98f, 0.52f, 0.26f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.98f, 0.52f, 0.26f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.98f, 0.52f, 0.26f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.98f, 0.52f, 0.26f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.98f, 0.52f, 0.26f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.98f, 0.52f, 0.26f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.98f, 0.52f, 0.26f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.98f, 0.52f, 0.26f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}


const std::map<std::string, ImFont*>& ImGuiHandler::getFontMap() {
	return m_fonts;
}

ImFont* ImGuiHandler::getFont(const std::string& font) {
	return m_fonts[font];
}

float ImGuiHandler::getFontScaling(const std::string& type) {
	if (m_scaling.find(type) != m_scaling.end()) {
		return m_scaling.at(type);
	}
	return 1.0f;
}

void ImGuiHandler::showMetrics(const bool show) {
	m_showMetrics = show;
}

void ImGuiHandler::addFonts() {
	ImGuiIO& io = ImGui::GetIO();
	const std::string defaultPath = "res/fonts/";
	const float size = 30;
	m_fonts["Beb60"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), size);


	m_scaling["BigHeader"] =	(70.0f/size); // 60
	m_scaling["Header0"] =	(60.0f/size); // 60
	m_scaling["Header1"] =	(42.0f/size); // 42
	m_scaling["Header2"] =	(40.0f/size); //40
	m_scaling["text"] =		(27.0f/size); // 27
	m_scaling["smalltext"] =		(24.0f/size); // 20





#ifndef USE_ONLY_ONE_FONT
	m_fonts["Beb24"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), 24);
	m_fonts["Beb27"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), 27);
	m_fonts["Beb30"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), 30);
	m_fonts["Beb40"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), 40);
	m_fonts["Beb50"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), 50);
	m_fonts["Beb60"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), 60);
	m_fonts["Beb70"] = io.Fonts->AddFontFromFileTTF(std::string(defaultPath + "BebasNeue.ttf").c_str(), 70);
#endif
}
