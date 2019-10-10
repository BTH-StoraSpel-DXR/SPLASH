#include "pch.h"
#include "Application.h"
#include "events/WindowResizeEvent.h"
#include "../../SPLASH/src/game/events/TextInputEvent.h"
#include "KeyBinds.h"
#include "graphics/geometry/Transform.h"
#include "Sail/TimeSettings.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/render/RenderSystem.h"


Application* Application::s_instance = nullptr;
std::atomic_bool Application::s_isRunning = true;

Application::Application(int windowWidth, int windowHeight, const char* windowTitle, HINSTANCE hInstance, API api) {
	// Set up instance if not set
	if (s_instance) {
		Logger::Error("Only one application can exist!");
		return;
	}
	s_instance = this;

	// Set up thread pool with two times as many threads as logical cores, or four threads if the CPU only has one core;
	// Note: this value might need future optimization
	unsigned int poolSize = std::max<unsigned int>(4, (10 * std::thread::hardware_concurrency()));
	m_threadPool = std::unique_ptr<ctpl::thread_pool>(SAIL_NEW ctpl::thread_pool(poolSize));

	// Set up window
	Window::WindowProps windowProps;
	windowProps.hInstance = hInstance;
	windowProps.windowWidth = windowWidth;
	windowProps.windowHeight = windowHeight;
	m_window = std::unique_ptr<Window>(Window::Create(windowProps));
	m_window->setWindowTitle(windowTitle);

	// Set up api
	m_api = std::unique_ptr<GraphicsAPI>(GraphicsAPI::Create());
	// Set up imgui handler
	m_imguiHandler = std::unique_ptr<ImGuiHandler>(ImGuiHandler::Create());

	// Initialize the window
	if (!m_window->initialize()) {
		OutputDebugString(L"\nFailed to initialize Win32Window\n");
		Logger::Error("Failed to initialize Win32Window!");
		return;
	}

	// Initialize the graphics API
	if (!m_api->init(m_window.get())) {
		OutputDebugString(L"\nFailed to initialize the graphics API\n");
		Logger::Error("Failed to initialize the graphics API!");
		return;
	}

	// Initialize Renderers
	m_rendererWrapper.initialize();
	ECS::Instance()->createSystem<RenderSystem>();

	// Initialize imgui
	m_imguiHandler->init();

	// Register devices to use raw input from hardware
	//m_input.registerRawDevices(*m_window.getHwnd());

	// Load the missing texture texture
	m_resourceManager.loadTexture("missing.tga");

}

Application::~Application() {
	delete Input::GetInstance();
}


// CAUTION: HERE BE DRAGONS!
// Moving around function calls in this function is likely to cause bugs and crashes
int Application::startGameLoop() {
	MSG msg = { 0 };
	m_fps = 0;
	// Start delta timer
	m_timer.startTimer();
	const INT64 startTime = m_timer.getStartTime();

	// Initialize key bindings
	KeyBinds::init();

	float currentTime = m_timer.getTimeSince<float>(startTime);
	float newTime = 0.0f;
	float delta = 0.0f;
	float accumulator = 0.0f;
	float secCounter = 0.0f;
	float elapsedTime = 0.0f;
	UINT frameCounter = 0;

	// Render loop, each iteration of it results in one rendered frame
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_KEYDOWN) {
				dispatchEvent(TextInputEvent(msg));
			}
		
		} else {
			// Handle window resizing
			if (m_window->hasBeenResized()) {
				UINT newWidth = m_window->getWindowWidth();
				UINT newHeight = m_window->getWindowHeight();
				bool isMinimized = m_window->isMinimized();
				// Send resize event
				dispatchEvent(WindowResizeEvent(newWidth, newHeight, isMinimized));
			}

			// Get delta time from last frame
			newTime = m_timer.getTimeSince<float>(startTime);
			delta = newTime - currentTime;
			currentTime = newTime;

			// Limit the amount of updates that can happen between frames to prevent the game from completely freezing
			// when the update is really slow for whatever reason.
			delta = std::min(delta, 4 * TIMESTEP);

			// Update fps counter
			secCounter += delta;
			accumulator += delta;
			frameCounter++;
			if (secCounter >= 1.0) {
				m_fps = frameCounter;
				frameCounter = 0;
				secCounter = 0.0;
			}

			// Update mouse deltas
			Input::GetInstance()->beginFrame();

			//UPDATES AUDIO
			//Application::getAudioManager()->updateAudio();

			// Quit on alt-f4
			if (Input::IsKeyPressed(KeyBinds::alt) && Input::IsKeyPressed(KeyBinds::f4))
				PostQuitMessage(0);

#ifdef _DEBUG
			/*if (m_input.getKeyboardState().Escape)
			PostQuitMessage(0);*/
			//if(delta > 0.0166)
			//	Logger::Warning(std::to_string(elapsedTime) + " delta over 0.0166: " + std::to_string(delta));
#endif
			// Process state specific input
			// NOTE: player movement is processed in update() except for mouse movement which is processed here
			processInput(delta);

			// Run the update if enough time has passed since the last update
			while (accumulator >= TIMESTEP) {
				accumulator -= TIMESTEP;
				fixedUpdate(TIMESTEP);
			}


			// alpha value used for the interpolation
			float alpha = accumulator / TIMESTEP;

			update(delta, alpha);

			// Render
			render(delta, alpha);
			//render(delta, 1.0f); // disable interpolation

			// Reset just pressed keys
			Input::GetInstance()->endFrame();
			
			// Do changes on the stack between states
			applyPendingStateChanges();
		}
	}

	s_isRunning = false;
	// Need to set all streams as 'm_isStreaming[i] = false' BEFORE stopping threads
	ECS::Instance()->stopAllSystems();
	m_threadPool->stop();
	ECS::Instance()->destroyAllSystems();
	return (int)msg.wParam;
}

std::string Application::getPlatformName() {
	return std::string(SAIL_PLATFORM);
}

Application* Application::getInstance() {
	if (!s_instance)
		Logger::Error("Application instance not set, you need to initialize the class which inherits from Application before calling getInstance().");
	return s_instance;
}

void Application::dispatchEvent(Event& event) {
	m_api->onEvent(event);
	Input::GetInstance()->onEvent(event);

	m_rendererWrapper.onEvent(event);
	
}

GraphicsAPI* const Application::getAPI() {
	return m_api.get();
}
Window* const Application::getWindow() {
	return m_window.get();
}
ImGuiHandler* const Application::getImGuiHandler() {
	return m_imguiHandler.get();
}
ResourceManager& Application::getResourceManager() {
	return m_resourceManager;
}

MemoryManager& Application::getMemoryManager() {
	return m_memoryManager;
}

RendererWrapper* Application::getRenderWrapper() {
	return &m_rendererWrapper;
}
StateStorage& Application::getStateStorage() {
	return this->m_stateStorage;
}
const UINT Application::getFPS() const {
	return m_fps;
}
