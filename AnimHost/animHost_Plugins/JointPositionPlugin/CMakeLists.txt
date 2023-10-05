qt_add_library(JointPositionPlugin
    JointPositionPlugin.cpp JointPositionPlugin.h
    JointPositionPlugin_global.h
)
target_include_directories(JointPositionPlugin PUBLIC
    ../PluginInterface
    ../../../glm
)


target_compile_definitions(JointPositionPlugin PUBLIC
    JOINTPOSITIONPLUGIN_LIBRARY
)

target_link_libraries(JointPositionPlugin PUBLIC
    PluginInterface
    AnimHostCore
)



