#pragma once

#include <memory>
#include "StateIdentifiers.h"
#include "../events/EventReceiver.h"
#include "../events/types/NetworkChangeStateEvent.h"

// Forward declare the StateStack
class StateStack;

// Base class for all states
class State : public EventReceiver {

	public:
		typedef std::unique_ptr<State> Ptr;

	public:
		State(StateStack& stack);
		virtual ~State();

		// Process input for the state.
		virtual bool processInput(float dt) = 0;
		// Updates the state - runs every frame
		virtual bool update(float dt, float alpha) = 0;
		// Updates the state - runs every tick
		virtual bool fixedUpdate(float dt);
		// Renders the state
		virtual bool render(float dt, float alpha) = 0;
		// Renders imgui
		virtual bool renderImgui(float dt);
		// Renders imgui used for debugging
		virtual bool renderImguiDebug(float dt);
		// Sends events to the state
		virtual bool onEvent(const Event& event) override;
		// Called at the end of the frame to reset the state before it changes
		virtual bool prepareStateChange() { return true; }

		virtual void onHostStateChangeRequest(const NetworkChangeStateEvent&);
//	protected:
		// Request the push of a new state to the stack next update
		void requestStackPush(States::ID stateID);
		// Request to pop the top state in the stack next update
		void requestStackPop();
		// Request to clear the stack in the next update
		void requestStackClear();

	private:
		// Pointer to the stack
		StateStack* m_stack;

};