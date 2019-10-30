cmake_minimum_required (VERSION 3.9)
project (apophenic)

set(BUILD_TESTS FALSE CACHE STRING "Build test binaries.")

include_directories(./)

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Werror")

set(APOPHENIC_HEADERS
	apophenic/MVC.hxx
	apophenic/StateAutomaton.hxx
)

install(FILES ${APOPHENIC_HEADERS} DESTINATION include/apophenic)

if(BUILD_TESTS)
	include(CTest)

	set(GTEST_LIB gtest)
	set(GMOCK_LIB gmock pthread)

	add_executable(test_mvc tests/test_mvc.cxx)
	target_link_libraries(test_mvc ${GTEST_LIB} ${GMOCK_LIB})
	add_test(mvc test_mvc)

	add_executable(test_automaton tests/test_automaton.cxx)
	target_link_libraries(test_automaton ${GTEST_LIB} ${GMOCK_LIB})
	add_test(automaton test_automaton)

endif(BUILD_TESTS)
