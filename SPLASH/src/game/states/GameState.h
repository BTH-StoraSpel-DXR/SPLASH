#pragma once

#include "Sail.h"
#include "../events/NetworkDisconnectEvent.h"
#include "../events/NetworkDroppedEvent.h"
#include "Sail/events/types/NetworkUpdateStateLoadStatus.h"

#include "../events/NetworkSerializedPackageEvent.h"
#include "../events/NetworkJoinedEvent.h"
#include "../events/NetworkNameEvent.h"
#include "../events/NetworkWelcomeEvent.h"
#include "Sail/entities/systems/SystemDeclarations.h"

class DX12DDSTexture;

class GameState final : public State {
public:
	GameState(StateStack& stack);
	~GameState();

	// Process input for the state ||
	virtual bool processInput(float dt) override;
	// Sends events to the state
	virtual bool onEvent(const Event& event) override;
	// Updates the state - Runs every frame
	virtual bool update(float dt, float alpha = 1.0f) override;
	// Updates the state - Runs every tick
	virtual bool fixedUpdate(float dt) override;
	// Renders the state
	virtual bool render(float dt, float alpha = 1.0f) override;
	// Renders imgui
	virtual bool renderImgui(float dt) override;
	// Renders imgui used for debugging
	virtual bool renderImguiDebug(float dt) override;

private:
	void initSystems(const unsigned char playerID);
	void initConsole();

	bool onResize(const WindowResizeEvent& event);
	bool onNetworkSerializedPackageEvent(const NetworkSerializedPackageEvent& event);
	bool onPlayerDisconnect(const NetworkDisconnectEvent& event);
	bool onPlayerDropped(const NetworkDroppedEvent& event);
	void onPlayerStateStatusChanged(const NetworkUpdateStateLoadStatus& event);
	bool onPlayerJoined(const NetworkJoinedEvent& event);
	void onToggleKillCam(const ToggleKillCamEvent& event);

	void shutDownGameState();

	// Where to updates the component systems. Responsibility can be moved to other places
	void updatePerTickKillCamComponentSystems(float dt);
	void updatePerTickComponentSystems(float dt);
	void updatePerFrameComponentSystems(float dt, float alpha);
	void runSystem(float dt, BaseComponentSystem* toRun);

	void createTestLevel(Shader* shader, Model* boundingBoxModel);
	void createBots(Model* boundingBoxModel, const std::string& characterModel, Model* projectileModel, Model* lightModel);
	void createLevel(Shader* shader, Model* boundingBoxModel);
	const std::string createCube(const glm::vec3& position);
	const std::string teleportToMap();
	const std::string toggleProfiler();

	void logSomeoneDisconnected(unsigned char id);

	void waitForOtherPlayers();

private:
	Application* m_app;
	// Camera
	PerspectiveCamera m_cam;

	// TODO: Only used for AI, should be removed once AI can target player in a better way.
	Entity* m_player;

	Entity* m_gameMusic = nullptr;
	Entity* m_ambiance = nullptr;
	bool m_readyRestartAmbiance = false;
	Systems m_componentSystems;
	LightSetup m_lights;
	Profiler m_profiler;
	RenderSettingsWindow m_renderSettingsWindow;
	LightDebugWindow m_lightDebugWindow;
	PlayerInfoWindow m_playerInfoWindow;
	WasDroppedWindow m_wasDroppedWindow;
	WaitingForPlayersWindow m_waitingForPlayersWindow;
	KillFeedWindow m_killFeedWindow;
	ECS_SystemInfoImGuiWindow m_ecsSystemInfoImGuiWindow;
	InGameGui m_inGameGui;
	ImGuiWindowFlags m_standaloneButtonflags;
	ImGuiWindowFlags m_backgroundOnlyflags;
	NetworkInfoWindow m_networkInfoImGuiWindow;

	size_t m_currLightIndex;

	bool m_paused = false;
	bool m_isSingleplayer = true;
	bool m_gameStarted = false;

	Octree* m_octree;
	bool m_showcaseProcGen;

	std::bitset<MAX_NUM_COMPONENTS_TYPES> m_currentlyWritingMask;
	std::bitset<MAX_NUM_COMPONENTS_TYPES> m_currentlyReadingMask;

	std::vector<std::future<BaseComponentSystem*>> m_runningSystemJobs;
	std::vector<BaseComponentSystem*> m_runningSystems;

	bool m_wasDropped = false;

	bool m_isInKillCamMode = false;
	std::string m_wasKilledBy = {};

#ifdef _PERFORMANCE_TEST
	void populateScene(Model* lightModel, Model* bbModel, Model* projectileModel, Shader* shader);

	std::vector<Entity::SPtr> m_performanceEntities;
#endif

};