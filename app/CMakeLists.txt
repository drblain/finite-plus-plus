add_executable(fsm-explorer ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(fsm-explorer PRIVATE fsm)

install(TARGETS fsm-explorer DESTINATION ${CMAKE_INSTALL_BINDIR})
