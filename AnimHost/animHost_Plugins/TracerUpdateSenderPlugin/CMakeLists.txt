qt_add_library(TracerUpdateSenderPlugin
    TracerUpdateSenderPlugin.cpp TracerUpdateSenderPlugin.h
    TracerUpdateSenderPlugin_global.h
    AnimHostMessageSender.cpp AnimHostMessageSender.h
    TickReceiver.h
)

set_target_properties (TracerUpdateSenderPlugin PROPERTIES
    FOLDER Plugins/DataOutput
)

target_include_directories(TracerUpdateSenderPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(TracerUpdateSenderPlugin PRIVATE
    TRACERUPDATESENDERPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../../../nzmqt/3rdparty/zmq/lib
)

target_link_libraries(TracerUpdateSenderPlugin PUBLIC
    PluginNodeInterface
)
