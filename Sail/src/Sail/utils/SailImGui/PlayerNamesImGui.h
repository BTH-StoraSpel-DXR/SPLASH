#pragma once

#include "SailImGuiWindow.h"

#include "Sail/entities/Entity.h"
#include "Sail/graphics/camera/Camera.h"

class PlayerNamesImGui : public SailImGuiWindow {
public:
	PlayerNamesImGui(bool showWindow = true);
	~PlayerNamesImGui();

	virtual void renderWindow() override;
	virtual void update(float dt);
	virtual void setLocalPlayer(Entity* player);
	virtual void addPlayerToDraw(Entity* player);
	virtual void clearPlayersToDraw();
	virtual void setCamera(Camera* camera);
	virtual void setMaxDistance(float dist);

private:
	struct PlayerName {
		Entity* playerEntity;
		float timeLeft;
	};

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize 
		| ImGuiWindowFlags_NoMove          | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus 
		| ImGuiWindowFlags_NoTitleBar      | ImGuiWindowFlags_AlwaysAutoResize 
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

	virtual glm::vec2 worldToScreen(const glm::vec3& coord);

	Entity* m_localPlayer = nullptr;
	std::vector<PlayerName> m_drawPlayers;

	Camera* m_camera = nullptr;
	float m_maxDist = 10;
};