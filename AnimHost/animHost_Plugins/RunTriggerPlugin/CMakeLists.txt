qt_add_library(RunTriggerPlugin
    RunTriggerPlugin.cpp RunTriggerPlugin.h
    RunTriggerPlugin_global.h
)

set_target_properties (RunTriggerPlugin PROPERTIES
    FOLDER Plugins/SignalGenerator
)

target_include_directories(RunTriggerPlugin PUBLIC
    ../PluginNodeInterface
    ../../../glm
)

target_compile_definitions(RunTriggerPlugin PUBLIC
    RUNTRIGGERPLUGIN_LIBRARY
)

target_link_libraries(RunTriggerPlugin PUBLIC
    PluginNodeInterface
    AnimHostCore
)
