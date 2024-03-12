
qt_add_library(TracerUpdateReceiverPlugin
    TracerUpdateReceiverPlugin.cpp TracerUpdateReceiverPlugin.h
    TracerUpdateReceiverPlugin_global.h
    TracerUpdateReceiver.h
)

set_target_properties (TracerUpdateReceiverPlugin PROPERTIES
    FOLDER Plugins/DataInput
)

target_include_directories(TracerUpdateReceiverPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(TracerUpdateReceiverPlugin PRIVATE
    TRACERUPDATERECEIVERPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../../../nzmqt/3rdparty/zmq/lib
)

target_link_libraries(TracerUpdateReceiverPlugin PUBLIC
    PluginNodeInterface
)
