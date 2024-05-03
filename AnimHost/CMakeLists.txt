cmake_minimum_required(VERSION 3.16...3.22)
set(CMAKE_TOOLCHAIN_FILE "../vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

project(AnimHost VERSION 1.0.0 LANGUAGES CXX)
set(target_name AnimHost)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE PATH "set config types" FORCE)
set(CMAKE_DEBUG_POSTFIX _d)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "set install prefix" FORCE)

set_property (GLOBAL PROPERTY USE_FOLDERS ON) 

add_subdirectory(core)
add_subdirectory(animHost_Plugins)
add_subdirectory(animHostApp)
