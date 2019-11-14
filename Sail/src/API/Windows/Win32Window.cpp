#include "pch.h"
#include "Win32Window.h"
#include "Sail/Application.h"
#include "Sail/events/EventDispatcher.h"
#include "Sail/events/types/WindowFocusChangedEvent.h"
#include "imgui.h"
#include "Win32Input.h"
#include <dxgi.h>
#include <wrl.h>
#include <examples/imgui_impl_dx12.h>

namespace {
	// Used to forward messages to user defined proc function
	Win32Window* g_pApp = nullptr;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (g_pApp) return g_pApp->MsgProc(hwnd, msg, wParam, lParam);
	else return DefWindowProc(hwnd, msg, wParam, lParam);
}

Window* Window::Create(const WindowProps& props) {
	return SAIL_NEW Win32Window(props);
}

Win32Window::Win32Window(const WindowProps& props)
	: Window(props)
	, m_hWnd(nullptr)
	, m_hInstance(props.hInstance)
	, m_windowTitle("Sail")
	, m_windowStyle(WS_OVERLAPPEDWINDOW) // Default window style
	, m_resized(false)
{
	g_pApp = this;

#ifdef _DEBUG
	m_windowTitle += " | Debug build";
#endif

}

Win32Window::~Win32Window() {
	
}


bool Win32Window::initialize() {

	// WNDCLASSEX
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.hInstance = m_hInstance;
	wcex.lpfnWndProc = MainWndProc;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"Win32Window";
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex)) {
		OutputDebugString(L"\nFailed to create window class\n");
		throw std::exception();
	}

	// Get the correct width and height (windows includes title bar in size)
	RECT r = { 0L, 0L, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
	AdjustWindowRect(&r, m_windowStyle, FALSE);
	UINT width = r.right - r.left;
	UINT height = r.bottom - r.top;

	UINT x = GetSystemMetrics(SM_CXSCREEN) / 2 - width / 2;
	UINT y = GetSystemMetrics(SM_CYSCREEN) / 2 - height / 2;

	std::wstring stemp = std::wstring(m_windowTitle.begin(), m_windowTitle.end());
	LPCWSTR title = stemp.c_str();

	m_hWnd = CreateWindow(L"Win32Window", title, m_windowStyle, x, y, width, height, NULL, NULL, m_hInstance, NULL);

	if (!m_hWnd) {
		OutputDebugString(L"\nFailed to create window\n");
		throw std::exception();
	}

	ShowWindow(m_hWnd, SW_SHOW);

	// Register raw input
	Input::GetInstance<Win32Input>()->registerRawDevices(m_hWnd);

	return true;

}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT Win32Window::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(m_hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:
	{
		// Set up window icon
		HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
		HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
		if (!hIcon) {
			SAIL_LOG_ERROR("Window icon could not be loaded");
		} else
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
		break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_ACTIVATEAPP:
	case WM_INPUT:
	case WM_MOUSEMOVE:
		Input::GetInstance<Win32Input>()->processMessage(msg, wParam, lParam);
		break;
	case WM_ACTIVATE:
		isWindowFocused = (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE);
		// Dispatch event
		EventDispatcher::Instance().emit(WindowFocusChangedEvent(isWindowFocused));
		break;
	case WM_SIZE:
	{
		if (wParam != SIZE_MINIMIZED){
			ImGui_ImplDX12_InvalidateDeviceObjects();
			ImGui_ImplDX12_CreateDeviceObjects();
		} else {
			isWindowMinimized = true;
			m_resized = true;
		}
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			if (width != windowWidth || height != windowHeight) {
				windowWidth = width;
				windowHeight = height;
				m_resized = true;
			}
		}
	}
		break;

	case WM_DPICHANGED:
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
			//const int dpi = HIWORD(wParam);
			//printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
			const RECT* suggested_rect = (RECT*)lParam;
			SetWindowPos(m_hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;

}

bool Win32Window::hasBeenResized() {
	bool ret = m_resized;
	m_resized = false;
	return ret;
}

void Win32Window::setWindowTitle(const std::string& title) {
	std::string newTitle = title;
	std::wstring ttle(newTitle.begin(), newTitle.end());
	SetWindowText(m_hWnd, ttle.c_str());
}

const HWND* Win32Window::getHwnd() const {
	return &m_hWnd;
}

DWORD Win32Window::getWindowStyle() const {
	return m_windowStyle;
}
