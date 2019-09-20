#pragma once
#include "api/Mesh.h"

#include "api/Input.h"
#include "api/GraphicsAPI.h"
#include "api/Window.h"
#include "api/ImGuiHandler.h"

#include "utils/Timer.h"
#include "resources/ResourceManager.h"
#include "events/IEventDispatcher.h"
#include "utils/StateStorage.h"

#include <future>

// Forward declarations
namespace ctpl {
	class thread_pool;
}

// TODO? Move elsewhere
const float TICKRATE = 100.0f;
const float TIMESTEP = 1.0f / TICKRATE;
#include "ai/pathfinding/NodeSystem.h"

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
	virtual void render(float dt, float alpha) = 0;
	virtual void dispatchEvent(Event& event) override;

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

	static std::string getPlatformName();
	static Application* getInstance();
	ImGuiHandler* const getImGuiHandler();
	ResourceManager& getResourceManager();
	NodeSystem* getNodeSystem();
	StateStorage& getStateStorage();
	const UINT getFPS() const;

private:
	static Application* s_instance;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<GraphicsAPI> m_api;
	std::unique_ptr<ImGuiHandler> m_imguiHandler;
	std::unique_ptr<ctpl::thread_pool> m_threadPool;
	ResourceManager m_resourceManager;
	std::unique_ptr<NodeSystem> m_nodeSystem;
	StateStorage m_stateStorage;

	// Timer
	Timer m_timer;
	UINT m_fps;

	static std::atomic_uint s_updateRunning;	
	static std::atomic_uint s_queuedUpdates;
	static std::atomic_bool s_isRunning;
};