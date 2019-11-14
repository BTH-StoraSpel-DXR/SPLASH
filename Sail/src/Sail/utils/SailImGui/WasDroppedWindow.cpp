#include "pch.h"
#include "WasDroppedWindow.h"
#include "imgui.h"

 WasDroppedWindow::WasDroppedWindow() {

}

 WasDroppedWindow::~WasDroppedWindow() {


 }

 void WasDroppedWindow::renderWindow() {
	//ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	flags |= ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin("ERROR", NULL, flags);
	ImGui::Text("CONNECTION TO HOST HAS BEEN LOST");
	ImGui::End();
 }
