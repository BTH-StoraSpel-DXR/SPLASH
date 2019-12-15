#include "game/Game.h"

// Entry point for windows subsystem
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
// Show console if compiled in debug
#ifdef _DEBUG
	AllocConsole();
	FILE* a;
	freopen_s(&a, "CONIN$", "r", stdin);
	freopen_s(&a, "CONOUT$", "w", stdout);
	freopen_s(&a, "CONOUT$", "w", stderr);
#endif

	// Check for memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	//_CrtSetBreakAlloc(903);
	

	Game game(hInstance);
	game.run();

	//OutputDebugString(L"\n========= Memory leak report =========\n");
	//_CrtDumpMemoryLeaks();
	//OutputDebugString(L"======================================\n\n");

	return 0;
}