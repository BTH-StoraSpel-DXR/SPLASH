#pragma once
class Window;
class GraphicsAPI;
class ImGuiHandler;
class ResourceManager;

#include "utils/Timer.h"
#include "events/IEventDispatcher.h"
#include <Windows.h>
#include <string>

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
	T* const getAPI() { return static_cast<T*>(m_api); }
	GraphicsAPI* const getAPI();

	template<typename T>
	T* const getWindow() { return static_cast<T*>(m_window); }
	Window* const getWindow();

	static std::string getPlatformName();
	static Application* getInstance();
	ImGuiHandler* const getImGuiHandler();
	ResourceManager* getResourceManager();
	const UINT getFPS() const;

private:
	static Application* m_instance;
	Window* m_window;
	GraphicsAPI* m_api;
	ImGuiHandler* m_imguiHandler;
	ResourceManager* m_resourceManager;

	Timer m_timer;
	UINT m_fps;
};