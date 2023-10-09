qt_add_library(JointVelocityPlugin
    JointVelocityPlugin.cpp JointVelocityPlugin.h
    JointVelocityPlugin_global.h
)

set_target_properties (JointVelocityPlugin PROPERTIES
    FOLDER Plugins/Operator
)

target_include_directories(JointVelocityPlugin PUBLIC
    ../PluginInterface
    ../../../glm
)

target_compile_definitions(JointVelocityPlugin PUBLIC
    JOINTVELOCITYPLUGIN_LIBRARY
)

target_link_libraries(JointVelocityPlugin PUBLIC
    PluginInterface
    AnimHostCore
)
