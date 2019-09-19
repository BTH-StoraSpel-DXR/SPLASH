#include "pch.h"
#include "Application.h"
#include "events/WindowResizeEvent.h"
#include "../../SPLASH/src/game/events/TextInputEvent.h" // ONLY 2 BITCH
#include "KeyCodes.h"
#include "api/Input.h"

Application* Application::m_instance = nullptr;

Application::Application(int windowWidth, int windowHeight, const char* windowTitle, HINSTANCE hInstance, API api) {

	// Set up instance if not set
	if (m_instance) {
		Logger::Error("Only one application can exist!");
		return;
	}
	m_instance = this;

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

	// Initalize the window
	if (!m_window->initialize()) {
		OutputDebugString(L"\nFailed to initialize Win32Window\n");
		Logger::Error("Failed to initialize Win32Window!");
		return;
	}

	// Initialize the graphics API
	if (!m_api->init(m_window.get())) {
		OutputDebugString(L"\nFailed to initialize the graphics API\n");
		Logger::Error("Failed to initialize the grahics API!");
		return;
	}

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

int Application::startGameLoop() {

	// Start delta timer
	m_timer.startTimer();
	
	MSG msg = {0};

	m_fps = 0;

	float secCounter = 0.f;
	float elapsedTime = 0.f;
	UINT frameCounter = 0;

	float updateTimer = 0.f;
	float timeBetweenUpdates = 1.f / 60.f;

	// TODO: move windows loop to api specific section

	// Main message loop
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
			float delta = static_cast<float>(m_timer.getFrameTime());
			delta = std::min(delta, 0.04f);

			// Update fps counter
			secCounter += delta;
			frameCounter++;

			if (secCounter >= 1) {
				m_fps = frameCounter;
				frameCounter = 0;
				secCounter = 0.f;
			}

			// Update input states
			//m_input.updateStates();

			// Update mouse deltas
			Input::GetInstance()->beginFrame();

			// Quit on alt-f4
			if (Input::IsKeyPressed(SAIL_KEY_MENU) && Input::IsKeyPressed(SAIL_KEY_F4))
				PostQuitMessage(0);

			processInput(delta);

			// Update
#ifdef _DEBUG
			/*if (m_input.getKeyboardState().Escape)
				PostQuitMessage(0);*/


			//if(delta > 0.0166)
			//	Logger::Warning(std::to_string(elapsedTime) + " delta over 0.0166: " + std::to_string(delta));
#endif
			updateTimer += delta;

			int maxCounter = 0;
		

			while (updateTimer >= timeBetweenUpdates) {
				if (maxCounter >= 4)
					break;
				update(timeBetweenUpdates);
				updateTimer -= timeBetweenUpdates;
				maxCounter++;
			}

			// Render
			render(delta);
			
			// Reset just pressed keys
			Input::GetInstance()->endFrame();
		}

	}

	return (int)msg.wParam;

}

std::string Application::getPlatformName() {
	return std::string(SAIL_PLATFORM);
}

Application* Application::getInstance() {
	if (!m_instance)
		Logger::Error("Application instance not set, you need to initialize the class which inherits from Application before calling getInstance().");
	return m_instance;
}

void Application::dispatchEvent(Event& event) {
	m_api->onEvent(event);
	Input::GetInstance()->onEvent(event);
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
const UINT Application::getFPS() const {
	return m_fps;
}
