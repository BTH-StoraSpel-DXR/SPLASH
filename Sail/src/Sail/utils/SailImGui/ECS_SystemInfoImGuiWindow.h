#pragma once

#include "SailImGuiWindow.h"
#include <map>

class Entity;

class ECS_SystemInfoImGuiWindow : public SailImGuiWindow {

public:
	ECS_SystemInfoImGuiWindow(bool showWindow = true);
	~ECS_SystemInfoImGuiWindow();

	void updateNumEntitiesInSystems(std::string systemName, int n);
	void updateNumEntitiesInECS(int n);

	virtual void renderWindow() override;

	void removeEntity(Entity* e);

private:
	std::map<std::string, int> m_nEntitiesInSystems;
	int m_nEntitiesInECS;

	Entity* selectedEntity;
	Entity* oldSelected;

};