#pragma once
#include "api/Mesh.h"

#include "api/Input.h"
#include "api/GraphicsAPI.h"
#include "api/Window.h"
#include "api/ImGuiHandler.h"
#include "api/Audio/audio.hpp"

#include "utils/Timer.h"
#include "resources/ResourceManager.h"
#include "MemoryManager/MemoryManager/src/MemoryManager.h"
#include "events/IEventDispatcher.h"
#include "utils/StateStorage.h"

class Application : public IEventDispatcher {

public:
	enum API {
		DX11, DX12
	};

public:
	Application(int windowWidth, int windowHeight, const char* windowTitle, HINSTANCE hInstance, API api = DX11);
	virtual ~Application();

	int startGameLoop();

	// Required methods
	virtual int run() = 0;
	virtual void processInput(float dt) = 0;
	virtual void update(float dt) = 0;
	virtual void render(float dt) = 0;
	virtual void dispatchEvent(Event& event) override;

	template<typename T>
	T* const getAPI() { return static_cast<T*>(m_api.get()); }
	GraphicsAPI* const getAPI();

	template<typename T>
	T* const getWindow() { return static_cast<T*>(m_window.get()); }
	Window* const getWindow();

	static std::string getPlatformName();
	static Application* getInstance();
	ImGuiHandler* const getImGuiHandler();
	ResourceManager& getResourceManager();
	MemoryManager& getMemoryManager();
	Audio* getAudioManager();
	StateStorage& getStateStorage();
	const UINT getFPS() const;

private:
	static Application* m_instance;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<GraphicsAPI> m_api;
	std::unique_ptr<ImGuiHandler> m_imguiHandler;
	ResourceManager m_resourceManager;
	MemoryManager m_memoryManager;
	Audio m_audioManager;
	StateStorage m_stateStorage;

	Timer m_timer;
	UINT m_fps;
};