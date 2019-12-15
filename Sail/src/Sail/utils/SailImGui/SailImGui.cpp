#include "pch.h"
#include "SailImGui.h"


bool SailImGui::TextButton(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

	auto text = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	auto hover = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
	auto active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));



	// Render
	const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	ImGui::RenderNavHighlight(bb, id);
	ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

	if (hovered) {
		ImGui::PushStyleColor(ImGuiCol_Text, hover);
	}
	else if (held || pressed) {
		ImGui::PushStyleColor(ImGuiCol_Text, active);
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Text, text);
	}
	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
	ImGui::PopStyleColor(4);
	// Automatically close popups
	//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
	//    CloseCurrentPopup();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
	return pressed;
}

bool SailImGui::InvertButton(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags) {

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

	// Render

	if (hovered) {
		auto text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		auto button = ImGui::GetStyleColorVec4(ImGuiCol_Button);

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, text);
		ImGui::PushStyleColor(ImGuiCol_Text, button);

	}
	else if (held || pressed) {
		//ImGui::PushStyleColor(ImGuiCol_Text, active);
	}
	else {
		//ImGui::PushStyleColor(ImGuiCol_Text, color);
	}

	const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	ImGui::RenderNavHighlight(bb, id);
	ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);






	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
	if (hovered) {
		ImGui::PopStyleColor(2);
	}
	// Automatically close popups
	//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
	//    CloseCurrentPopup();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
	return pressed;
}

void SailImGui::rText(const char* fmt, const float end, ...) {

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + end - ImGui::CalcTextSize(fmt).x
		- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

	va_list args;
	va_start(args, end);
	ImGui::TextV(fmt, args);
	va_end(args);
}

void SailImGui::cText(const char* fmt, const float end, ...) {
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((end- ImGui::GetCursorPosX() )*0.5f) - (ImGui::CalcTextSize(fmt).x * 0.5f) -   ImGui::GetStyle().ItemSpacing.x*0.5f);

	va_list args;
	va_start(args, end);
	ImGui::TextV(fmt, args);
	va_end(args);
}

void SailImGui::HeaderText(const char* fmt, ...) {	
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt, args);
	va_end(args);
	ImGui::PopStyleColor();
}

void SailImGui::cHeaderText(const char* fmt, const float end, ...) {	
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((end - ImGui::GetCursorPosX()) * 0.5f) - (ImGui::CalcTextSize(fmt).x * 0.5f) - ImGui::GetStyle().ItemSpacing.x * 0.5f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt, args);
	va_end(args);
	ImGui::PopStyleColor();
}

