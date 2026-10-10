add_executable(finite-plus-plus ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(finite-plus-plus PRIVATE fsm)

install(TARGETS finite-plus-plus DESTINATION ${CMAKE_INSTALL_BINDIR})
