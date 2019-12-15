#include "pch.h"
#include "Application.h"
#include "events/types/WindowResizeEvent.h"
#include "../SPLASH/src/game/events/TextInputEvent.h"
#include "KeyBinds.h"
#include "KeyCodes.h"
#include "graphics/geometry/Transform.h"
#include "TimeSettings.h"
#include "entities/ECS.h"
#include "entities/systems/Systems.h"
#include "events/EventDispatcher.h"

// If this is defined then fixed update will run every frame which speeds up/slows down
// the game depending on how fast the program can run
//#define PERFORMANCE_SPEED_TEST

Application* Application::s_instance = nullptr;
std::atomic_bool Application::s_isRunning = true;

Application::Application(int windowWidth, int windowHeight, const char* windowTitle, HINSTANCE hInstance, API api) : 
	m_settingStorage("res/data/settings.saildata")
	{
	// Set up instance if not set
	if (s_instance) {
		SAIL_LOG_ERROR("Only one application can exist!");
		return;
	}
	s_instance = this;

	// Set up console
	m_consoleCommands = std::make_unique<ConsoleCommands>(false);
	
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
		SAIL_LOG_ERROR("Failed to initialize Win32Window!");
		return;
	}

	// Initialize the graphics API
	if (!m_api->init(m_window.get())) {
		OutputDebugString(L"\nFailed to initialize the graphics API\n");
		SAIL_LOG_ERROR("Failed to initialize the graphics API!");
		return;
	}

	// Initialize Renderers
	m_rendererWrapper.initialize();
	ECS::Instance()->createSystem<BeginEndFrameSystem>();
	ECS::Instance()->createSystem<BoundingboxSubmitSystem>();
	ECS::Instance()->createSystem<MetaballSubmitSystem<RenderInActiveGameComponent>>();
	ECS::Instance()->createSystem<ModelSubmitSystem<RenderInActiveGameComponent>>();
	ECS::Instance()->createSystem<GUISubmitSystem>();
	ECS::Instance()->createSystem<LevelSystem>()->generateMap();
	m_settingStorage.gameSettingsDynamic["map"]["count"].maxVal = ECS::Instance()->getSystem<LevelSystem>()->powerUpSpawnPoints.size();

	// Initialize imgui
	m_imguiHandler->init();

	// Register devices to use raw input from hardware
	//m_input.registerRawDevices(*m_window.getHwnd());

	m_chatWindow = std::make_unique<ChatWindow>(true);
	ImVec2 size(400, 300);
	m_chatWindow->setSize(size);
	m_chatWindow->setPosition(ImVec2(30,m_window->getWindowHeight()-size.y-30));

}

Application::~Application() {
	m_settingStorage.saveToFile("res/data/settings.saildata");
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
	loadKeybinds();

	m_delta = 0.0f;
	float currentTime = m_timer.getTimeSince<float>(startTime);
	float newTime = 0.0f;
	float accumulator = 0.0f;
	float secCounter = 0.0f;
	float elapsedTime = 0.0f;
	UINT frameCounter = 0;
	float fixedUpdateStartTime = 0.0f;

	// Render loop, each iteration of it results in one rendered frame
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_KEYDOWN) {
				EventDispatcher::Instance().emit(TextInputEvent(msg));
			}
		
		} else {
			// Handle window resizing
			if (m_window->hasBeenResized()) {
				UINT newWidth = m_window->getWindowWidth();
				UINT newHeight = m_window->getWindowHeight();
				bool isMinimized = m_window->isMinimized();
				// Send resize event
				EventDispatcher::Instance().emit(WindowResizeEvent(newWidth, newHeight, isMinimized));
			}

			// Get delta time from last frame
			newTime = m_timer.getTimeSince<float>(startTime);
			m_delta = newTime - currentTime;
			currentTime = newTime;


			// Update fps counter
			secCounter += m_delta;
			frameCounter++;
			if (secCounter >= 1.0) {
				m_fps = frameCounter;
				frameCounter = 0;
				secCounter = 0.0;
			}

			// Update mouse deltas
			Input::GetInstance()->beginFrame();

			// Quit on alt-f4
			if (Input::IsKeyPressed(KeyBinds::ALT_KEY) && Input::IsKeyPressed(KeyBinds::F4_KEY)) {
				PostQuitMessage(0);
			}

#ifdef _DEBUG
			/*if (Input::WasKeyJustPressed(SAIL_KEY_ESCAPE)) {
				PostQuitMessage(0);
			}*/
			//if(m_delta > 0.0166)
			//	SAIL_LOG_WARNING(std::to_string(elapsedTime) + " delta over 0.0166: " + std::to_string(m_delta));
#endif
			// Process state specific input
			processInput(m_delta);


			// Limit the amount of updates that can happen between frames to prevent the game from completely freezing
			// when the update is really slow for whatever reason.
			// Allows fixed update to run twice in a row before rendering a frame so the game
			// will slow down if (3*FPS < TICKRATE)
			accumulator += std::min(m_delta, 3.01f * TIMESTEP);


			// Run the update if enough time has passed since the last update
#ifndef PERFORMANCE_SPEED_TEST
			while (accumulator >= TIMESTEP) {
#endif

				accumulator -= TIMESTEP;

				fixedUpdateStartTime = m_timer.getTimeSince<float>(startTime);
				fixedUpdate(TIMESTEP);
				m_fixedUpdateDelta = m_timer.getTimeSince<float>(startTime) - fixedUpdateStartTime;


#ifndef PERFORMANCE_SPEED_TEST
			}
			// alpha value used for the interpolation
			float alpha = accumulator / TIMESTEP;
#else
			float alpha = 1.0f; // disable interpolation
#endif

			update(m_delta, alpha);

			// Render
			render(m_delta, alpha);

			// Reset just pressed keys
			Input::GetInstance()->endFrame();
			
			// Do changes on the stack between states
			applyPendingStateChanges();
		}
	}

	s_isRunning = false;
	// Need to set all streams as 'm_isStreaming[i] = false' BEFORE stopping threads
	
	// NOTE: 'stopAllSystems()' / 'destroyAllSystems()' SHOULD already have been called
	ECS::Instance()->stopAllSystems();
	m_threadPool->stop();
	ECS::Instance()->destroyAllSystems();
	return (int)msg.wParam;
}

void Application::setCurrentCamera(Camera* camera) {
	m_cameraRef = camera;
}

std::string Application::getPlatformName() {
	return std::string(SAIL_PLATFORM);
}

Application* Application::getInstance() {
	if (!s_instance)
		SAIL_LOG_ERROR("Application instance not set, you need to initialize the class which inherits from Application before calling getInstance().");
	return s_instance;
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

ConsoleCommands& Application::getConsole() {
	return *m_consoleCommands;
}

SettingStorage& Application::getSettings() {
	return m_settingStorage;
}

ChatWindow* Application::getChatWindow() {
	return m_chatWindow.get();
}

Camera* Application::getCurrentCamera() const {
	return m_cameraRef;
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
float Application::getDelta() const {
	return m_delta;
}

float Application::getFixedUpdateDelta() const {
	return m_fixedUpdateDelta;
}

void Application::loadKeybinds() {

	KeyBinds::MOVE_FORWARD = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["forward"].value;
	KeyBinds::MOVE_BACKWARD = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["backward"].value;
	KeyBinds::MOVE_RIGHT = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["right"].value;
	KeyBinds::MOVE_LEFT = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["left"].value;
	KeyBinds::MOVE_UP = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["up"].value;
	KeyBinds::MOVE_DOWN = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["down"].value;
	KeyBinds::SPRINT = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["sprint"].value;
	KeyBinds::THROW_CHARGE = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["throw"].value;
	KeyBinds::LIGHT_CANDLE = (int)m_settingStorage.applicationSettingsDynamic["keybindings"]["light"].value;
}

void Application::startAudio() {
	// Create the system if it doesn't exist.
	ECS* ecsPtr = ECS::Instance();
	if (!ecsPtr->getSystem<AudioSystem>()) {
		ecsPtr->createSystem<AudioSystem>();

		while (audioEntitiesQueue.size() != 0) {
			// Add the one in the back
			ecsPtr->addEntityToSystems(audioEntitiesQueue.back());
			// Pop it
			audioEntitiesQueue.pop_back();
		}
		
		SAIL_LOG("Audio was created successfully");
	}
}

void Application::addToAudioComponentQueue(Entity* ac) {
	this->audioEntitiesQueue.push_back(ac);
}

