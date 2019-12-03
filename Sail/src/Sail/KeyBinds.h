#pragma once

namespace KeyBinds {
	// Initializes the key codes
	void init();

	// Player controls
	extern int MOVE_FORWARD;
	extern int MOVE_BACKWARD;
	extern int MOVE_RIGHT;
	extern int MOVE_LEFT;
	extern int MOVE_UP;
	extern int MOVE_DOWN;
	extern int SPRINT;
	extern int SHOOT;
	extern int THROW_CHARGE;
	extern int SPRAY;
	extern int LIGHT_CANDLE;
	extern int SHOW_IN_GAME_MENU;

	// GameState
	extern int ADD_LIGHT;
	extern int TOGGLE_BOUNDINGBOXES;
	extern int TEST_RAYINTERSECTION;
	extern int TEST_FRUSTUMCULLING;
	extern int TOGGLE_AI_FOLLOWING;
	extern int SET_DIRECTIONAL_LIGHT;
	extern int TOGGLE_CONSOLE;
	extern int TOGGLE_CONSOLE_US;
	extern int RELOAD_SHADER;
	extern int REMOVE_OLDEST_LIGHT;
	extern int LIGHT_CANDLE_1;
	extern int LIGHT_CANDLE_2;
	extern int DISABLE_CURSOR;
	extern int TOGGLE_SPHERE;	// Collision
	extern int TOGGLE_SUN;
	extern int TOGGLE_ROOM_LIGHTS;

	// Application
	extern int ALT_KEY;
	extern int F4_KEY;

	// Audio
	extern int _0;
	extern int _1;
	extern int _2;
	extern int _3;
	extern int _9;

	// StateStack
	extern int TOGGLE_IMGUI;

	// LobbyState
	extern int SEND_MESSAGE;

	// DX12RaytracingRenderer
	extern int RELOAD_DXR_SHADER;


	// Keybinds used for debugging
		// TODO: Move all debugging keybinds here
	extern int SPECTATOR_DEBUG;
};
