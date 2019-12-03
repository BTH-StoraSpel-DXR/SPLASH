#pragma once

#include "Sail.h"
#include "Sail/entities/systems/SystemDeclarations.h"

class DX12DDSTexture;

class PBRTestState final : public State {
public:
	PBRTestState(StateStack& stack);
	~PBRTestState();

	// Process input for the state
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

private:
	bool onResize(const WindowResizeEvent& event);

private:
	Application* m_app;
	// Camera
	PerspectiveCamera m_cam;
	FlyingCameraController m_camController;
	Octree* m_octree;

	const std::string createCube(const glm::vec3& position);

	Systems m_componentSystems;
	LightSetup m_lights;
	Profiler m_profiler;
	RenderSettingsWindow m_renderSettingsWindow;
	LightDebugWindow m_lightDebugWindow;

	size_t m_currLightIndex;
	// For use by non-deterministic entities
	const float* pAlpha = nullptr;

	std::unique_ptr<Model> m_cubeModel;
	std::unique_ptr<Model> m_planeModel;
	std::unique_ptr<Model> m_sphereModel;
	DX12DDSTexture* m_testDDSTexture;

};