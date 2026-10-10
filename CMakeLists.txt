cmake_minimum_required(VERSION 3.28)

project(finite-plus-plus LANGUAGES CXX)

include(GNUInstallDirs)

add_subdirectory(libfsm)
add_subdirectory(app)

include(package.cmake)
