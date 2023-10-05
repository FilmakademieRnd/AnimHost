
qt_add_library(TracerUpdateSenderPlugin
    TracerUpdateSenderPlugin.cpp TracerUpdateSenderPlugin.h
    TracerUpdateSenderPlugin_global.h
    AnimHostMessageSender.cpp AnimHostMessageSender.h
    TickReceiver.h
)
target_include_directories(TracerUpdateSenderPlugin PUBLIC
    ../PluginNodeInterface
    ../../../glm
    ../../core
)

target_compile_definitions(TracerUpdateSenderPlugin PUBLIC
    TRACERUPDATESENDERPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../../../nzmqt/3rdparty/zmq/lib
)

target_link_libraries(TracerUpdateSenderPlugin PUBLIC
    PluginNodeInterface
    Qt::Core
    AnimHostCore
    libzmq-mt-4_3_5
)
