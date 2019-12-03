#include "pch.h"
#include "KeyBinds.h"
#include "Sail/KeyCodes.h"
#include "Sail/MouseButtonCodes.h"

// Defines the variables
namespace KeyBinds {

	// Player controls
	int MOVE_FORWARD;
	int MOVE_BACKWARD;
	int MOVE_RIGHT;
	int MOVE_LEFT;
	int MOVE_UP;
	int MOVE_DOWN;
	int SPRINT;
	int SHOOT;
	int SPRAY;
	int LIGHT_CANDLE;
	int SHOW_IN_GAME_MENU;

	// GameState
	int ADD_LIGHT;
	int TOGGLE_BOUNDINGBOXES;
	int TEST_RAYINTERSECTION;
	int TEST_FRUSTUMCULLING;
	int TOGGLE_AI_FOLLOWING;
	int SET_DIRECTIONAL_LIGHT;
	int TOGGLE_CONSOLE;
	int TOGGLE_CONSOLE_US;
	int RELOAD_SHADER;
	int REMOVE_OLDEST_LIGHT;
	int LIGHT_CANDLE_1;
	int LIGHT_CANDLE_2;
	int THROW_CHARGE;
	int DISABLE_CURSOR;
	int TOGGLE_SPHERE;
	int TOGGLE_SUN;
	int TOGGLE_ROOM_LIGHTS;

	// Application
	int ALT_KEY;
	int F4_KEY;

	// Audio
	int _0;
	int _1;
	int _2;
	int _3;
	int _9;

	// StateStack
	int TOGGLE_IMGUI;

	// LobbyState
	int SEND_MESSAGE;

	// DX12RaytracingRenderer
	int RELOAD_DXR_SHADER;

	// Debugging
	int SPECTATOR_DEBUG;
}

void KeyBinds::init() {
	
	// Player controls
	MOVE_FORWARD       = SAIL_KEY_W;
	MOVE_BACKWARD      = SAIL_KEY_S;
	MOVE_RIGHT         = SAIL_KEY_D;
	MOVE_LEFT          = SAIL_KEY_A;
	MOVE_UP            = SAIL_KEY_SPACE;
	MOVE_DOWN          = SAIL_KEY_CONTROL;
	SPRINT             = SAIL_KEY_SHIFT;
	SHOOT              = SAIL_MOUSE_LEFT_BUTTON;
	THROW_CHARGE	   = SAIL_KEY_F;
	SPRAY              = SAIL_KEY_L;
	LIGHT_CANDLE       = SAIL_KEY_R;
	SHOW_IN_GAME_MENU  = SAIL_KEY_ESCAPE;

	// GameState
	ADD_LIGHT             = SAIL_KEY_E;
	TOGGLE_BOUNDINGBOXES  = SAIL_KEY_B;
	TEST_RAYINTERSECTION  = SAIL_KEY_O;
	TEST_FRUSTUMCULLING   = SAIL_KEY_N;
	TOGGLE_AI_FOLLOWING   = SAIL_KEY_H;
	SET_DIRECTIONAL_LIGHT = SAIL_KEY_G;
	TOGGLE_CONSOLE        = SAIL_KEY_OEM_5;
	TOGGLE_CONSOLE_US     = SAIL_KEY_OEM_5; // SAIL_KEY_OEM_3 was original keybind
	RELOAD_SHADER         = SAIL_KEY_F6;
	REMOVE_OLDEST_LIGHT   = SAIL_KEY_X;
	LIGHT_CANDLE_1        = SAIL_KEY_Z;
	LIGHT_CANDLE_2        = SAIL_KEY_V;
	DISABLE_CURSOR        = SAIL_MOUSE_RIGHT_BUTTON;
	TOGGLE_SPHERE         = SAIL_KEY_C;
	TOGGLE_SUN            = SAIL_KEY_P;
	TOGGLE_ROOM_LIGHTS	  = SAIL_KEY_Q;

	// Application
	ALT_KEY = SAIL_KEY_MENU;		// Did not know what to call these binds
	F4_KEY  = SAIL_KEY_F4;

	// Audio
	_0 = SAIL_KEY_0;		// Did not know what to call these binds
	_1 = SAIL_KEY_1;
	_2 = SAIL_KEY_2;
	_3 = SAIL_KEY_3;
	_9 = SAIL_KEY_9;

	// StateStack
	TOGGLE_IMGUI = SAIL_KEY_F10;

	// LobbyState
	SEND_MESSAGE = SAIL_KEY_RETURN;

	// DX12RaytracingRenderer
	RELOAD_DXR_SHADER = SAIL_KEY_F5;

	// Debugging
	SPECTATOR_DEBUG = SAIL_KEY_M;
}
