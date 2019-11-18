#include "pch.h"
#include "CustomImGuiComponents.h"
#include "imgui_internal.h"

using namespace ImGui;

void CustomImGui::CustomProgressBar(float fraction, const ImVec2& size_arg, const char* overlay, const ImVec4& bgColor_full/*, const ImVec4& bgColor_empty*/) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
	ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, 0))
		return;

	// Render
	fraction = ImSaturate(fraction);
	RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
	bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
	const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
	RenderRectFilledRangeH(window->DrawList, bb, ColorConvertFloat4ToU32(bgColor_full), 0.0f, fraction, style.FrameRounding);

	// Default displaying the fraction as percentage string, but user can override it
	char overlay_buf[32];
	if (!overlay) {
		ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", fraction * 100 + 0.01f);
		overlay = overlay_buf;
	}

	ImVec2 overlay_size = CalcTextSize(overlay, NULL);
	if (overlay_size.x > 0.0f)
		RenderTextClipped(ImVec2(ImClamp(fill_br.x + style.ItemSpacing.x, bb.Min.x, bb.Max.x - overlay_size.x - style.ItemInnerSpacing.x), bb.Min.y), bb.Max, overlay, NULL, &overlay_size, ImVec2(0.0f, 0.5f), &bb);
}
