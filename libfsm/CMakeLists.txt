add_library(fsm SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Automaton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Nfa.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Dfa.cpp)

target_include_directories(fsm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(TARGETS fsm DESTINATION ${CMAKE_INSTALL_LIBDIR}/fsm)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
