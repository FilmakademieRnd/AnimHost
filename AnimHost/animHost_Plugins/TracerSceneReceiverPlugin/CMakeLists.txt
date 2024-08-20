set(target_name TracerSceneReceiverPlugin)

qt_add_library(${target_name}
    TracerSceneReceiverPlugin.cpp TracerSceneReceiverPlugin.h
    TracerSceneReceiverPlugin_global.h
    SceneReceiver.cpp SceneReceiver.h
)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/DataInput
)

target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(${target_name} PRIVATE
    TRACERSCENERECEIVERPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../../../nzmqt/3rdparty/zmq/lib
)

target_link_libraries(${target_name} PUBLIC
    PluginInterface
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
