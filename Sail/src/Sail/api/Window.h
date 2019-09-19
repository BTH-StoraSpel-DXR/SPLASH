#pragma once
#include <string>
#include <Windows.h>

class Window {
public:
	friend class Application;

	struct WindowProps {
		HINSTANCE hInstance;
		unsigned int windowWidth = 1280;
		unsigned int windowHeight = 720;
	};
public:
	static Window* Create(const WindowProps& props);
	Window(const WindowProps& props) : windowWidth(props.windowWidth), windowHeight(props.windowHeight), isWindowMinimized(false), isWindowFocused(false) {}
	virtual ~Window() {}

	virtual bool initialize() = 0;

	virtual inline void setWindowTitle(const std::string& title) = 0;
	virtual inline unsigned int getWindowWidth() const { return windowWidth; };
	virtual inline unsigned int getWindowHeight() const { return windowHeight; };
	virtual inline bool isMinimized() const { return isWindowMinimized; };
	virtual inline bool isFocused() const { return isWindowFocused; };

protected:
	unsigned int windowWidth;
	unsigned int windowHeight;
	bool isWindowMinimized;
	bool isWindowFocused;

private:
	// NOTE: this method is only used internally by the sail and should not be called by the user
	// Returns true if the window has been resized
	// after the last call of this method
	virtual bool hasBeenResized() = 0;

};