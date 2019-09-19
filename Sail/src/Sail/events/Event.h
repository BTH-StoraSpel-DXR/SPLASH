#pragma once

class Event {
public:
	enum Type {
		WINDOW_RESIZE,
		WINDOW_FOCUS_CHANGED,
		POTATO,
		TEXTINPUT,
		NETWORK_JOINED,
		NETWORK_DISCONNECT,
		NETWORK_CHAT,
		NETWORK_WELCOME,
		NETWORK_START_GAME
	};
public:
	Event(Type type);
	~Event();

	Type getType() const;

private:
	Type m_type;

};