#pragma once
#include "imgui.h"

namespace CustomImGui {
	// size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
	void CustomProgressBar(float fraction, const ImVec2& size_arg, const char* overlay, const ImVec4& bgColor_full/*, const ImVec4& bgColor*/);
}