#pragma once
#include "api/Mesh.h"

#include "api/Input.h"
#include "api/GraphicsAPI.h"
#include "api/Window.h"
#include "api/ImGuiHandler.h"

#include "utils/Timer.h"
#include "utils/SailImGui/ConsoleCommands.h"
#include "utils/Storage/SettingStorage.h"
#include "utils/StateStorage.h"
#include "resources/ResourceManager.h"
#include "MemoryManager/MemoryManager/src/MemoryManager.h"
#include "RendererWrapper.h"
#include "Sail/../../SPLASH/src/ImGuiWindows/Chat/ChatWindow.h"
#include <ctpl/ctpl_stl.h>

#include <future>

class Camera;

// Forward declarations
namespace ctpl {
	class thread_pool;
}

class Application {

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
	virtual void update(float dt, float alpha) = 0;
	virtual void fixedUpdate(float dt) = 0;
	virtual void render(float dt, float alpha) = 0;
	virtual void applyPendingStateChanges() = 0;

	template<typename T>
	T* const getAPI() { return static_cast<T*>(m_api.get()); }
	GraphicsAPI* const getAPI();

	template<typename T>
	T* const getWindow() { return static_cast<T*>(m_window.get()); }
	Window* const getWindow();

	// Pass-through functions for pushing jobs (functions, lambdas, etc.) to the thread pool.
	// The first parameter of all jobs must be int id which becomes the id of the running thread.
	// Returns std::future containing the return type of the job.
	//
	// EXAMPLE USAGE:
	// To push a member function from another member function use a lambda which captures this and other relevant arguments:
	//     float f = 10.0f;
	//     pushJobToThreadPool([this,f](int id) { return this->memberFunction(id, f); });
	//
	// To push a member function in another object use a lambda which captures that object and other relevant arguments:
	//     Object obj;
	//     pushJobToThreadPool([&obj, f](int id) { return obj.memberFunction(id, f); });
	//
	// See https://github.com/vit-vit/CTPL for other examples with standalone functions, lambdas, etc.
	template<typename F, typename... Rest>
	auto pushJobToThreadPool(F&& f, Rest&& ... rest)->std::future<decltype(f(0, rest...))> {
		return m_threadPool->push(f, std::forward<Rest>(rest)...);
	}
	template<typename F>
	auto pushJobToThreadPool(F&& f)->std::future<decltype(f(0))> {
		return m_threadPool->push(f);
	}

	virtual void setCurrentCamera(Camera* camera);

	static std::string getPlatformName();
	static Application* getInstance();
	ImGuiHandler* const getImGuiHandler();
	ResourceManager& getResourceManager();
	ConsoleCommands& getConsole();
	SettingStorage& getSettings();
	ChatWindow* getChatWindow();
	Camera* getCurrentCamera() const;

	MemoryManager& getMemoryManager();
	RendererWrapper* getRenderWrapper();
	StateStorage& getStateStorage();
	const UINT getFPS() const;
	float getDelta() const;
	float getFixedUpdateDelta() const;
	void loadKeybinds();

private:

	static Application* s_instance;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<GraphicsAPI> m_api;
	std::unique_ptr<ImGuiHandler> m_imguiHandler;
	std::unique_ptr<ctpl::thread_pool> m_threadPool;
	std::unique_ptr<ConsoleCommands> m_consoleCommands;
	ResourceManager m_resourceManager;
	RendererWrapper m_rendererWrapper;

	MemoryManager m_memoryManager;
	StateStorage m_stateStorage;
	SettingStorage m_settingStorage;
	std::unique_ptr<ChatWindow> m_chatWindow;
	Camera* m_cameraRef;
	// Timer
	Timer m_timer;
	UINT m_fps;
	float m_delta;
	float m_fixedUpdateDelta;

	static std::atomic_bool s_isRunning;
};