#pragma once

#include "Sail.h"

class RenderSystem;
class OctreeAddRemoverSystem;
class UpdateBoundingBoxSystem;

class PerformanceTestState : public State {
public:
	PerformanceTestState(StateStack& stack);
	~PerformanceTestState();

	// Process input for the state
	virtual bool processInput(float dt) override;
	// Sends events to the state
	virtual bool onEvent(Event& event) override;
	// Updates the state - Runs every frame
	virtual bool update(float dt, float alpha = 1.0f) override;
	// Updates the state - Runs every tick
	virtual bool fixedUpdate(float dt) override;
	// Renders the state
	virtual bool render(float dt, float alpha = 1.0f) override;
	// Renders imgui
	virtual bool renderImgui(float dt) override;



private:
	bool onResize(WindowResizeEvent& event);
	bool renderImguiConsole(float dt);
	bool renderImguiProfiler(float dt);
	bool renderImGuiRenderSettings(float dt);
	bool renderImGuiLightDebug(float dt);

private:
	struct Systems {
		RenderSystem* renderSystem = nullptr;
		EntityAdderSystem* entityAdderSystem = nullptr;
		EntityRemovalSystem* entityRemovalSystem = nullptr;
		OctreeAddRemoverSystem* octreeAddRemoverSystem = nullptr;
		UpdateBoundingBoxSystem* updateBoundingBoxSystem = nullptr;
	};

	Application* m_app;
	// Camera
	PerspectiveCamera m_cam;
	FlyingCameraController m_camController;
	Octree* m_octree;

	const std::string createCube(const glm::vec3& position);

	Systems m_componentSystems;
	LightSetup m_lights;
	ConsoleCommands m_cc;
	Profiler m_profiler;

	size_t m_currLightIndex;
	// For use by non-deterministic entities
	const float* pAlpha = nullptr;

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

	std::unique_ptr<Model> m_cubeModel;
	std::unique_ptr<Model> m_planeModel;
	std::unique_ptr<Model> m_sphereModel;

};