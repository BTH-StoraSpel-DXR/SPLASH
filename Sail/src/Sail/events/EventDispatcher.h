#pragma once
#include <vector>
#include "Event.h"

/* Notes */
// =======================================================================================================================
// | This event dispatcher should never be declared manually, since it's a singleton                                     |
// | Preferably, include and use it only in .cpp-files                                                                   |
// -----------------------------------------------------------------------------------------------------------------------
// | When emitting an event, create and send it event in one go                                                          |
// -----------------------------------------------------------------------------------------------------------------------
// | I.e. EventDispatcher::Instance()->Emit(CreatedPositionEvent(playerPos));                                            |
// =========================================================================================================================================
// | The class receiving events needs to inherit from EventReceiver and override void onEvent(const Event& e)                              |
// | In onEvent(const Event& e), the event type needs to be checked, preferably with a switch statement                                    |
// | If the type matches with one of the cases, the value of the event needs to be managed as needed. A call to a function is recommended  |
// -----------------------------------------------------------------------------------------------------------------------------------------
// | I.e.                                                                                                                                  |
// | void onEvent(const Event& event) {                                                                                                    |
// |     switch(event.type) {                                                                                                                  |
// |     case CREATED_POSITION:                                                                                                            |
// |         onCreatedPosition((const CreatedPositionEvent&)event);  // Use pos in this function here                                      |
// |         break;                                                                                                                        |
// |     }                                                                                                                                 |
// | }   
// |
// =========================================================================================================================================
// | Receivers can only receive events of a type they're subscribed to                                                     |
// | Receivers can be subscribed to an event without having a case for it, or vice versa, but preferrably not              |
// | One receiver can be subscribed to multiple event types, but will only become a subscriber once per type               |
// -------------------------------------------------------------------------------------------------------------------------
// | A receiver can either subscribe itself (this), or have another class do it. The same applies with unsubscribing       |
// -------------------------------------------------------------------------------------------------------------------------
// | I.e. 	EventDispatcher::Instance().subscribe(Event::Type::INSANITY_SYSTEM_UPDATE_INSANITY, this);                     |
// =========================================================================================================================
// | It is recommended, but not always necessary, for a class to unsubscribe itself from all events when it's destroyed |
// | Otherwise the dispatcher might attempt to call onEvent() on a destroyed object                                     |
// ======================================================================================================================

class EventReceiver;

class EventDispatcher final {
public:
	static EventDispatcher& Instance() {
		static EventDispatcher ed;
		return ed;
	}

	~EventDispatcher() = default;

	void emit(const Event& e);
	void subscribe(const Event::Type& type, EventReceiver* subscriber);
	void unsubscribe(const Event::Type& type, EventReceiver* subscriber);

private:
	EventDispatcher() = default;
	bool subscribed(const Event::Type& type, EventReceiver* subscriber);

private:
	std::vector<EventReceiver*> m_subscribers[static_cast<size_t>(Event::Type::NR_OF_EVENTS)];
};
