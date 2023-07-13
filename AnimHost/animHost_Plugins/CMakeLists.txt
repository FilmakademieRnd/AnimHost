cmake_minimum_required(VERSION 3.16...3.22)

project(AnimHostPlugins VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE PATH "set config types" FORCE)
set(CMAKE_DEBUG_POSTFIX _d)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "set install prefix" FORCE)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui Widgets)
qt_standard_project_setup()

add_subdirectory(PluginInterface)
add_subdirectory(ExamplePlugin)

