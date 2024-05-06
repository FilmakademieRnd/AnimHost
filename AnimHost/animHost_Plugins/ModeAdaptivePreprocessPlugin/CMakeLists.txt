set(target_name ModeAdaptivePreprocessPlugin)

qt_add_library(${target_name} SHARED
    ModeAdaptivePreprocessPlugin.cpp ModeAdaptivePreprocessPlugin.h
    ModeAdaptivePreprocessPlugin_global.h
)
target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(${target_name} PRIVATE
    MODEADAPTIVEPREPROCESSPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginNodeInterface
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
