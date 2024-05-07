set(target_name TracerUpdateSenderPlugin)

qt_add_library(${target_name}
    TracerUpdateSenderPlugin.cpp TracerUpdateSenderPlugin.h
    TracerUpdateSenderPlugin_global.h
    AnimHostMessageSender.cpp AnimHostMessageSender.h
    TickReceiver.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/DataOutput
)

target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(${target_name} PRIVATE
    TRACERUPDATESENDERPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../../../nzmqt/3rdparty/zmq/lib
)

target_link_libraries(${target_name} PUBLIC
    PluginNodeInterface
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
