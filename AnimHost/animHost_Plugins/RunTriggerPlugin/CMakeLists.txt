set(target_name RunTriggerPlugin)

qt_add_library(${target_name}
    RunTriggerPlugin.cpp RunTriggerPlugin.h
    RunTriggerPlugin_global.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/SignalGenerator
)

target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(${target_name} PRIVATE
    RUNTRIGGERPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginNodeInterface
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
