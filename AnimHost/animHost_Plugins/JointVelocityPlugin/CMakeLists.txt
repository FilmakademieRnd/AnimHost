set(target_name JointVelocityPlugin)

qt_add_library(${target_name}
    JointVelocityPlugin.cpp JointVelocityPlugin.h
    JointVelocityPlugin_global.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/Operator
)

target_include_directories(${target_name} PUBLIC
    ../PluginInterface
)

target_compile_definitions(${target_name} PRIVATE
    JOINTVELOCITYPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
