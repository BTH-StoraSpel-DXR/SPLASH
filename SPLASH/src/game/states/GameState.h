#pragma once

#include "Sail.h"
#include "../controllers/PlayerController.h"

class AiSystem;
class AnimationSystem;
class CandleSystem;
class EntityRemovalSystem;
class LifeTimeSystem;
class LightSystem;
class OctreeAddRemoverSystem;
class PhysicSystem;
class PrepareUpdateSystem;

class GameState : public State {
public:
	GameState(StateStack& stack);
	~GameState();

	// Process input for the state
	virtual bool processInput(float dt) override;
	// Sends events to the state
	virtual bool onEvent(Event& event) override;
	// Updates the state
	virtual bool update(float dt) override;
	// Renders the state
	virtual bool render(float dt, float alpha) override;
	// Renders imgui
	virtual bool renderImgui(float dt) override;



private:
	bool onResize(WindowResizeEvent& event);
	bool renderImguiConsole(float dt);
	bool renderImguiProfiler(float dt);
	bool renderImGuiRenderSettings(float dt);
	bool renderImGuiLightDebug(float dt);
	// Where to updates the component systems. Responsibility can be moved to other places
	void updatePerTickComponentSystems(float dt);


private:
	struct Systems {
		AiSystem* aiSystem = nullptr;
		AnimationSystem* animationSystem = nullptr;
		CandleSystem* candleSystem = nullptr;
		EntityRemovalSystem* entityRemovalSystem = nullptr;
		LifeTimeSystem* lifeTimeSystem = nullptr;
		LightSystem* lightSystem = nullptr;
		OctreeAddRemoverSystem* octreeAddRemoverSystem = nullptr;
		PhysicSystem* physicSystem = nullptr;
		UpdateBoundingBoxSystem* updateBoundingBoxSystem = nullptr;
		PrepareUpdateSystem* prepareUpdateSystem = nullptr;
	};

	Application* m_app;
	// Camera
	PerspectiveCamera m_cam;
	//FlyingCameraController m_camController;
	PlayerController m_playerController;

	const std::string createCube(const glm::vec3& position);

	Systems m_componentSystems;
	Scene m_scene;
	LightSetup m_lights;
	ConsoleCommands m_cc;
	Profiler m_profiler;


	// ImGUI profiler data
	float m_profilerTimer = 0.f;
	int m_profilerCounter = 0;
	float* m_virtRAMHistory;
	float* m_physRAMHistory;
	float* m_cpuHistory;
	float* m_vramUsageHistory;
	float* m_frameTimesHistory;
	std::string m_virtCount;
	std::string m_physCount;
	std::string m_vramUCount;
	std::string m_cpuCount;
	std::string m_ftCount;

	// Uncomment this to enable vram budget visualization
	//std::string m_vramBCount;
	//float* m_vramBudgetHistory;

	std::unique_ptr<Model> m_cubeModel;
	std::unique_ptr<Model> m_planeModel;
	

	std::unique_ptr<Model> m_boundingBoxModel;

	Octree* m_octree;
	bool m_disableLightComponents;
};