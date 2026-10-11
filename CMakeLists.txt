cmake_minimum_required(VERSION 3.28)

project(finite-plus-plus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

add_subdirectory(libfsm)
add_subdirectory(app)

include(package.cmake)
