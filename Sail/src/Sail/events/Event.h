#pragma once

struct Event {
	enum class Type {
		WINDOW_RESIZE,
		WINDOW_FOCUS_CHANGED,
		POTATO,
		TEXTINPUT,
		NETWORK_LAN_HOST_FOUND,
		NETWORK_JOINED,
		NETWORK_DISCONNECT,
		NETWORK_CHAT,
		NETWORK_WELCOME,
		NETWORK_SWITCH_STATE,
		NETWORK_NAME,
		NETWORK_DROPPED,
		NETWORK_SERIALIZED_DATA_RECIEVED,
		PLAYER_CANDLE_DEATH,
		GAME_OVER,
		RESET_WATER,
		SETTINGS_UPDATED,

		NR_OF_EVENTS		// Needs to be last, and no type above can set their values manually
	};

	Event(const Type& _type) : type(_type) {}
	virtual ~Event() = default;

	const Type type;
};