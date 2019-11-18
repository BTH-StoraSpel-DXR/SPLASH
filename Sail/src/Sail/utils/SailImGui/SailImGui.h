#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"



namespace SailImGui {
	bool TextButton(const char* label, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0);
	bool InvertButton(const char* label, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0);
	void rText(const char* fmt, const float end, ...);
	void cText(const char* fmt, const float end, ...);
	void HeaderText(const char* fmt, ...);
}