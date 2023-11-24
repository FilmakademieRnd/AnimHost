qt_add_library(JointPositionPlugin
    JointPositionPlugin.cpp JointPositionPlugin.h
    JointPositionPlugin_global.h
)

set_target_properties (JointPositionPlugin PROPERTIES
    FOLDER Plugins/Operator
)

target_include_directories(JointPositionPlugin PUBLIC
    ../PluginInterface
)


target_compile_definitions(JointPositionPlugin PRIVATE
    JOINTPOSITIONPLUGIN_LIBRARY
)

target_link_libraries(JointPositionPlugin PUBLIC
    PluginInterface
)



