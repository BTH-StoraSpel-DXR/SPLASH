#include "pch.h"
#include "ECS_SystemInfoImGuiWindow.h"
#include "imgui.h"

#include "../../entities/ECS.h"
#include "../../entities/components/Components.h"

#include "Sail/Application.h"

ECS_SystemInfoImGuiWindow::ECS_SystemInfoImGuiWindow(bool showWindow) {
	selectedEntity = nullptr;
	oldSelected = nullptr;
}

ECS_SystemInfoImGuiWindow::~ECS_SystemInfoImGuiWindow() {
}

void ECS_SystemInfoImGuiWindow::updateNumEntitiesInSystems(std::string systemName, int n) {
	m_nEntitiesInSystems[systemName] = n;
}

void ECS_SystemInfoImGuiWindow::updateNumEntitiesInECS(int n) {
	m_nEntitiesInECS = n;
}

void ECS_SystemInfoImGuiWindow::renderWindow() {
#ifdef DEVELOPMENT
	ECS* ecs = ECS::Instance();
	if (!ecs) {
		return;
	}
	const ECS::SystemMap* systemMap = &ecs->getSystems();
	static float windowWeight = 1.0f;
	
	

	if (ImGui::Begin("ECS System Entities")) {
		ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("smalltext"));

		oldSelected = selectedEntity;
		if(ImGui::BeginChild("SYSTEMS", ImVec2(260, 0), false)) {
			ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("smalltext"));


			for (auto const& [key, val] : *systemMap) {
				std::string name(key.name());

				if (ImGui::CollapsingHeader(std::string(name.substr(name.find(" "), std::string::npos) + ": " + std::to_string(val->getEntities().size())).c_str())) {
					ImGui::Separator();
					val->imguiPrint(&selectedEntity);
					if (oldSelected != selectedEntity) {
						ImGui::EndChild();
						ImGui::End();
						return;
					}
					for (const auto& e : val->getEntities()) {
						if (name == "EntityRemovalSystem") {
							if (e == selectedEntity) {
								selectedEntity = nullptr;
								ImGui::EndChild();
								ImGui::End();
								return;
							}
						}
						if (ImGui::Selectable(std::string(e->getName()+"("+ std::to_string(e->getID())+")").c_str(), selectedEntity == e)) {
							
							selectedEntity = (selectedEntity == e ? nullptr : e);
						}
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::BeginChild("ENTITY", ImVec2(0, 0), false)) {
			ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("smalltext"));
			if (selectedEntity) {
				ImGui::Text(std::string("Name: " + selectedEntity->getName()).c_str());
				ImGui::Text(std::string("ID: " + std::to_string(selectedEntity->getID())).c_str());
				if (selectedEntity->getParent()) {
					if (ImGui::Selectable(std::string("Parent: " + selectedEntity->getParent()->getName()).c_str(), selectedEntity == selectedEntity->getParent())) {
						selectedEntity = selectedEntity->getParent();
						ImGui::EndChild();
						ImGui::End();
						return;
					}
				}
				for (const auto& child : selectedEntity->getChildEntities()) {
					if (ImGui::Selectable(std::string("Child: " + child->getName()).c_str(), selectedEntity == child)) {
						selectedEntity = child;
						ImGui::EndChild();
						ImGui::End();
						return;
					}
				}
				ImGui::Separator();
				const BaseComponent::Ptr* components = selectedEntity->getComponents();
				for (unsigned int index = 0; index < BaseComponent::nrOfComponentTypes(); index++) {
					if (BaseComponent* ptr = components[index].get()) {
						std::string name(ptr->getName());
						
						if (ImGui::CollapsingHeader(std::string(name.substr(name.find(" ") + 1, std::string::npos)).c_str())) {
							ImGui::BeginGroup();
							ImGui::Indent(16.0f);
							ptr->imguiRender(&selectedEntity);
							if (selectedEntity != oldSelected) {
								SAIL_LOG("switched");
								ImGui::Unindent(16.0f);
								ImGui::EndGroup();
								ImGui::EndChild();
								ImGui::End();
								return;
							}
							ImGui::Unindent(16.0f);
							ImGui::EndGroup();
						}
					}
				}
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
#endif
	return;
}

void ECS_SystemInfoImGuiWindow::removeEntity(Entity* e) {
	if (e == selectedEntity) {
		selectedEntity = nullptr;
	}
	if (e == oldSelected) {
		oldSelected = nullptr;
	}
}
