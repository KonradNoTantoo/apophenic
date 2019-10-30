#include <string>
#include <vector>
#include <list>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "apophenic/StateAutomaton.hxx"


enum eStates
{
		ENTRY
	,	WAITING_FOR_INPUT
	,	CALCULATING
	,	ERROR_HANDLING
	,	END
};


enum eTransitions
{
		INITIALIZED
	,	INITIALIZATION_ERROR
	,	USER_INPUT
	,	CALCULATION_OVER
	,	RUNTIME_ERROR
	,	INPUT_ERROR
	,	USER_INTERRUPT
};


class TestMachine
	: public ap::Automaton
		<
			TestMachine
		,	eStates
		,	eTransitions
		>
{
	typedef ap::Automaton
		<
				TestMachine
			,	eStates
			,	eTransitions
		> Automaton;
	friend class Automaton;

protected:
	template<eStates ST> void enter_state();
	template<eTransitions TR> void on_event();
	template<eStates ST, eTransitions TR> void exit_state();

public:
	/*
	 * In production code, starting_state<>() should be called by
	 * constructors, to ensure that our automaton is not left
	 * badly initialized.
	 * Here, for test purposes, we decouple construction and full
	 * initialization.
	 */
	TestMachine() { /* starting_state<ENTRY>();*/ }
	void set_starting_state() { starting_state<ENTRY>(); }

	using Automaton::state;

	MOCK_CONST_METHOD0(begin_initialization, void());
	MOCK_CONST_METHOD0(prompt_for_input, void());
	MOCK_CONST_METHOD0(calculate, void());
	MOCK_CONST_METHOD0(display_wait, void());
	MOCK_CONST_METHOD0(display_result, void());
	MOCK_CONST_METHOD0(display_error, void());
	MOCK_CONST_METHOD0(cleanup, void());
	MOCK_CONST_METHOD0(exit_success, void());
	MOCK_CONST_METHOD0(exit_failure, void());

	void on_initialization_finished() { transition<INITIALIZED,ENTRY>(); }
	void on_initialization_error() { transition<INITIALIZATION_ERROR,ENTRY>(); }
	void on_input() { transition<USER_INPUT,WAITING_FOR_INPUT,ERROR_HANDLING>(); }
	void on_calculation_finished() { transition<CALCULATION_OVER,CALCULATING>(); }
	void on_interruption() { transition<USER_INTERRUPT,ENTRY,WAITING_FOR_INPUT,CALCULATING,ERROR_HANDLING>(); }
	void on_runtime_error() { transition<RUNTIME_ERROR,CALCULATING>(); }
	void on_bad_input() { transition<INPUT_ERROR,WAITING_FOR_INPUT,WAITING_FOR_INPUT>(); }

	void forbidden_event() { transition<USER_INPUT,ENTRY>(); }
};


// state machine implementation
////////////////////////////

namespace ap
{

#define ALLOW_TRANSITION( _start_state, _end_state, _event ) \
template<> \
template<> \
struct Automaton< TestMachine, eStates, eTransitions>::Transition<  _start_state,  _event > \
{ \
	static constexpr bool ALLOWED = true; \
	static constexpr eStates END_STATE = _end_state ; \
}

ALLOW_TRANSITION( ENTRY, WAITING_FOR_INPUT, INITIALIZED );
ALLOW_TRANSITION( ENTRY, END, INITIALIZATION_ERROR );
ALLOW_TRANSITION( ENTRY, END, USER_INTERRUPT );

ALLOW_TRANSITION( WAITING_FOR_INPUT, CALCULATING, USER_INPUT );
ALLOW_TRANSITION( WAITING_FOR_INPUT, ERROR_HANDLING, INPUT_ERROR );
ALLOW_TRANSITION( WAITING_FOR_INPUT, END, USER_INTERRUPT );

ALLOW_TRANSITION( CALCULATING, WAITING_FOR_INPUT, CALCULATION_OVER );
ALLOW_TRANSITION( CALCULATING, ERROR_HANDLING, RUNTIME_ERROR );
ALLOW_TRANSITION( CALCULATING, WAITING_FOR_INPUT, USER_INTERRUPT );

ALLOW_TRANSITION( ERROR_HANDLING, WAITING_FOR_INPUT, USER_INPUT );
ALLOW_TRANSITION( ERROR_HANDLING, END, INPUT_ERROR );
ALLOW_TRANSITION( ERROR_HANDLING, END, USER_INTERRUPT );

#undef ALLOW_TRANSITION

}


// enter_state and specializations
////////////////////////////

template<eStates ST>
void TestMachine::enter_state() {}

template<>
void TestMachine::enter_state<ENTRY>() { begin_initialization(); }

template<>
void TestMachine::enter_state<WAITING_FOR_INPUT>() { prompt_for_input(); }

template<>
void TestMachine::enter_state<CALCULATING>() { calculate(); }

template<>
void TestMachine::enter_state<ERROR_HANDLING>() { display_error(); }

template<>
void TestMachine::enter_state<END>() { cleanup(); }


// on_event and specializations
////////////////////////////

template<eTransitions TR>
void TestMachine::on_event() {}

template<>
void TestMachine::on_event<CALCULATION_OVER>() { display_result(); }

template<>
void TestMachine::on_event<USER_INPUT>() { display_wait(); }

template<>
void TestMachine::on_event<USER_INTERRUPT>() { display_wait(); }


// exit_state and specializations
////////////////////////////

template<eStates ST, eTransitions TR>
void TestMachine::exit_state() {}

template<>
void TestMachine::exit_state<END,INITIALIZATION_ERROR>() { exit_failure(); }

template<>
void TestMachine::exit_state<END,INPUT_ERROR>() { exit_failure(); }

template<>
void TestMachine::exit_state<END,USER_INTERRUPT>() { exit_success(); }


struct AutomatonFixture : ::testing::Test
{
	TestMachine _machine;

	void SetUp()
	{
		EXPECT_CALL(_machine, begin_initialization())
			.Times(1);
		_machine.set_starting_state();
	}
};


TEST_F(AutomatonFixture, starting_state)
{
	EXPECT_EQ(ENTRY, _machine.state());
}


TEST_F(AutomatonFixture, transitions)
{
	EXPECT_CALL(_machine, prompt_for_input())
		.Times(2);
	EXPECT_CALL(_machine, calculate())
		.Times(2);
	EXPECT_CALL(_machine, display_wait())
		.Times(2);
	EXPECT_CALL(_machine, display_result())
		.Times(1);
	EXPECT_CALL(_machine, display_error())
		.Times(1);

	_machine.on_initialization_finished();
	_machine.on_input();
	_machine.on_calculation_finished();
	_machine.on_input();
	_machine.on_runtime_error();

	EXPECT_EQ(ERROR_HANDLING, _machine.state());
}


TEST_F(AutomatonFixture, bad_transition)
{
	EXPECT_THROW(_machine.on_input(), TestMachine::EWrongState);
}


TEST_F(AutomatonFixture, forbidden_transition)
{
	EXPECT_THROW(_machine.forbidden_event(), TestMachine::EUnauthorizedTransition);
}


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
