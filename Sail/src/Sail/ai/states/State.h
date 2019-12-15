#pragma once

#include <vector>
#include "Sail/utils/Utils.h"

typedef unsigned int FSMStateID;

extern FSMStateID global_fsmStateID;

class Entity;

namespace FSM {
	struct Transition;

	class BaseState {
	public:
		BaseState() {}
		virtual ~BaseState() {
			for ( int i = 0; i < m_transitions.size(); i++ ) {
				Memory::SafeDelete(m_transitions[i].first);
			}
		}

		static FSMStateID createID() {
			return global_fsmStateID++;
		}

		/*
			The base state will handle deletion of any transition added to it

			@param transition the transition to be tested
			@param toState the state to transition to if transition is successful
		*/
		void addTransition(Transition* transition, BaseState* toState) {
			m_transitions.emplace_back(transition, toState);
		}

		/*
			@return the transitions available from this state
		*/
		std::vector<std::pair<Transition*, BaseState*>>& getTransitions() {
			return m_transitions;
		}

		virtual void update(float dt, Entity* entity) = 0;

		virtual void reset(Entity* entity) = 0;

		virtual void init(Entity* entity) = 0;
		
#ifdef DEVELOPMENT
		const std::string getName() {
			return typeid(*this).name();
		}
#endif

	protected:
		std::vector<std::pair<Transition*, BaseState*>> m_transitions;
		float m_stateTimer;

	};

	template <typename StateType>
	class State : public BaseState {
	public:
		State() {}
		virtual ~State() {}

		static const FSMStateID ID;

	};
	
	struct Transition {
		void addBoolCheck(bool* toCheck, bool value) {
			boolChecks.emplace_back(toCheck, value);
		}
		void addFloatLessThanCheck(float* toCheck, float value) {
			floatLessThanChecks.emplace_back(toCheck, value);
		}
		void addFloatEqualCheck(float* toCheck, float value) {
			floatEqualChecks.emplace_back(toCheck, value);
		}
		void addFloatGreaterThanCheck(float* toCheck, float value) {
			floatGreaterThanChecks.emplace_back(toCheck, value);
		}

		std::vector<std::pair<bool*, bool>> boolChecks;
		std::vector<std::pair<float*, float>> floatLessThanChecks;
		std::vector<std::pair<float*, float>> floatEqualChecks;
		std::vector<std::pair<float*, float>> floatGreaterThanChecks;
	};



	/*
		Defines the constant static ID of each state type at compile time
	*/
	template<typename StateType>
	const FSMStateID State<StateType>::ID = BaseState::createID();
}