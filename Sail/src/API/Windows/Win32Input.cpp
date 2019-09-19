#include "pch.h"
#include "Win32Input.h"
#include <windowsx.h>
#include "sail/Application.h"
#include "Win32Window.h"
#include "Sail/api/GraphicsAPI.h"

Input* Input::m_Instance = SAIL_NEW Win32Input();

Win32Input::Win32Input()
	: m_mouseButtons{ false }
	, m_keys{ false }
	, m_frameKeys{ false }
	, m_frameMouseButtons{ false }
	, m_mousePos(0, 0)
	, m_cursorHidden(false)
	, m_stopInput(false)
{
}

Win32Input::~Win32Input() {}

bool Win32Input::isKeyPressedImpl(int keycode) {
	return m_keys[keycode];
}

bool Win32Input::wasKeyJustPressedImpl(int keycode) {
	return m_frameKeys[keycode];
}

bool Win32Input::isMouseButtonPressedImpl(int button) {
	return m_mouseButtons[button];
}

bool Win32Input::wasMouseButtonJustPressedImpl(int button) {
	return m_frameMouseButtons[button];
}

glm::ivec2 Win32Input::getMousePositionImpl() {
	return m_mousePos;
}

glm::ivec2 Win32Input::getMouseDeltaImpl() {
	return m_mouseDelta;
}

void Win32Input::hideCursorImpl(bool hide) {
	m_cursorHidden = hide;
	if (hide)
		while (ShowCursor(false) >= 0);
	else
		ShowCursor(true);
}

bool Win32Input::isCursorHiddenImpl() {
	return m_cursorHidden;
}

void Win32Input::beginFrame() {
	m_mouseDelta.x = m_mouseDX;
	m_mouseDelta.y = m_mouseDY;
	m_mouseDX = 0;
	m_mouseDY = 0;

	// Always center cursor if hidden
	if (isCursorHiddenImpl()) {
		auto* wnd = Application::getInstance()->getWindow<Win32Window>();
		POINT p;
		p.x = wnd->getWindowWidth() / 2;
		p.y = wnd->getWindowHeight() / 2;
		ClientToScreen(*wnd->getHwnd(), &p);
		SetCursorPos(p.x, p.y);
	}
}

void Win32Input::endFrame() {
	std::fill(m_frameMouseButtons, m_frameMouseButtons + sizeof(m_frameMouseButtons), false);
	std::fill(m_frameKeys, m_frameKeys + sizeof(m_frameKeys), false);
}

void Win32Input::registerRawDevices(HWND hwnd) {
	// Register mouse for raw input
	RAWINPUTDEVICE Rid[1];

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = RIDEV_INPUTSINK;   // adds HID mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = hwnd;

	if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {
		//registration failed. Call GetLastError for the cause of the error
		OutputDebugString(L"Could not register mouse for raw input");
	}
}

void Win32Input::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {

	if (m_stopInput) return;

	// The lParam thing ignores repeated keystrokes
	// Handle ALT+ENTER:
	if (msg == WM_SYSKEYDOWN && (wParam == VK_RETURN) && (lParam & (1 << 29))) {
		Application::getInstance()->getAPI()->toggleFullscreen();
	}

	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
		m_keys[wParam] = true;
		m_frameKeys[wParam] = true;
	} else if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {
		m_keys[wParam] = false;
		m_frameKeys[wParam] = false;
	} else if (msg == WM_MOUSEMOVE) {
		m_mousePos.x = GET_X_LPARAM(lParam);
		m_mousePos.y = GET_Y_LPARAM(lParam);
	} else if (msg == WM_INPUT) {
		// Mouse button input 

		UINT dwSize = 0;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
		LPBYTE lpb = SAIL_NEW BYTE[dwSize];
		if (lpb == nullptr) {
			return;
		}

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
			OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

		RAWINPUT* raw = (RAWINPUT*)lpb;
		if (raw->header.dwType == RIM_TYPEMOUSE) {
			// Accumulate deltas until the next frame
			m_mouseDX += raw->data.mouse.lLastX;
			m_mouseDY += raw->data.mouse.lLastY;

			// Update mouse buttons
			bool down = true;
			int sailCode = 0;
			for (int i = 0x0001; i <= 0x0200; i <<= 1) {
				if ((i & raw->data.mouse.usButtonFlags) == i) {
					m_mouseButtons[sailCode] = down;
					m_frameMouseButtons[sailCode] = down;
				}
				if (!down) sailCode++;
				down = !down;
			}

		}

		delete[] lpb;
	}
}

bool Win32Input::onEvent(Event& event) {
	auto handleFocusChange = [&](WindowFocusChangedEvent& event) {
		if (event.isFocused()) {
			m_stopInput = false;
		} else {
			// show hidden cursor and remove any keys or buttons marked as pressed
			HideCursor(false);
			std::fill_n(m_mouseButtons, SAIL_NUM_MOUSE_BUTTONS, false);
			std::fill_n(m_frameMouseButtons, SAIL_NUM_MOUSE_BUTTONS, false);
			std::fill_n(m_keys, SAIL_NUM_KEYS, false);
			std::fill_n(m_frameKeys, SAIL_NUM_KEYS, false);

			m_stopInput = true;
		}
		return true;
	};
	EventHandler::dispatch<WindowFocusChangedEvent>(event, handleFocusChange);
	return true;
}
