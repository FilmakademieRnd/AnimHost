include(C:/DEV/AnimHost/cmake_build/.qt/QtDeploySupport-Release.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/core-plugins-Release.cmake" OPTIONAL)
set(__QT_DEPLOY_ALL_MODULES_FOUND_VIA_FIND_PACKAGE "ZlibPrivate;EntryPointPrivate;Core;Gui;Widgets")

qt6_deploy_runtime_dependencies(
    EXECUTABLE C:/DEV/AnimHost/cmake_build/Release/core.exe
    GENERATE_QT_CONF
)
