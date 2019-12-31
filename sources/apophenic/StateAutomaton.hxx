#ifndef STATE_AUTOMATON_HXX
#define STATE_AUTOMATON_HXX


namespace ap
{



template<typename Implementor, typename TState, typename TEvent>
class Automaton
{
public:
	struct EUnauthorizedTransition
	{
		TState _state;
		TEvent _event;
	};

	struct EWrongState
	{
		TState _state;
	};

protected:
	TState state() const { return _state; }

	template<TState BEGIN_STATE, TEvent EVENT>
	struct Transition
	{
		static constexpr bool ALLOWED = false;
		static constexpr TState END_STATE = BEGIN_STATE;
	};

	template<TState BEGIN_STATE, TEvent EVENT, bool ALLOWED>
	struct TransitionApplier;

	template<TState BEGIN_STATE, TEvent EVENT>
	struct TransitionApplier<BEGIN_STATE, EVENT, true>
	{
		void operator()(Automaton & atm) const
		{
			static_cast<Implementor&>(atm).template exit_state<BEGIN_STATE, EVENT>();
			atm._state = Transition<BEGIN_STATE,EVENT>::END_STATE;
			static_cast<Implementor&>(atm).template on_event<EVENT>();
			static_cast<Implementor&>(atm).template enter_state<Transition<BEGIN_STATE,EVENT>::END_STATE>();
		}
	};

	template<TState BEGIN_STATE, TEvent EVENT>
	struct TransitionApplier<BEGIN_STATE, EVENT, false>
	{
		void operator()(Automaton &) const { throw EUnauthorizedTransition{BEGIN_STATE, EVENT}; }
	};

	template<TState STATE>
	void starting_state()
	{
		_state = STATE;
		static_cast<Implementor&>(*this).template enter_state<STATE>();
	};

	template<TEvent EVENT, TState BEGIN_STATE>
	void transition()
	{
		if ( BEGIN_STATE == _state )
		{
			TransitionApplier<BEGIN_STATE,EVENT,Transition<BEGIN_STATE,EVENT>::ALLOWED>()(*this);
		}
		else throw EWrongState{BEGIN_STATE};
	}

	template<TEvent EVENT, TState FIRST_STATE, TState NEXT_STATE, TState... STATES>
	void transition()
	{
		if ( FIRST_STATE == _state )
		{
			TransitionApplier<FIRST_STATE,EVENT,Transition<FIRST_STATE,EVENT>::ALLOWED>()(*this);
		}
		else transition<EVENT,NEXT_STATE,STATES...>();
	}

private:
	TState _state;
};



};

#endif // STATE_AUTOMATON_HXX
