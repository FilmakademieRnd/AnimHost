
qt_add_library(TracerSceneReceiverPlugin
    TracerSceneReceiverPlugin.cpp TracerSceneReceiverPlugin.h
    TracerSceneReceiverPlugin_global.h
    SceneReceiver.cpp SceneReceiver.h
)

set_target_properties (TracerSceneReceiverPlugin PROPERTIES
    FOLDER Plugins/DataInput
)

target_include_directories(TracerSceneReceiverPlugin PUBLIC
    ../PluginNodeInterface
)

target_compile_definitions(TracerSceneReceiverPlugin PRIVATE
    TRACERSCENERECEIVERPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../../../nzmqt/3rdparty/zmq/lib
)

target_link_libraries(TracerSceneReceiverPlugin PUBLIC
    PluginNodeInterface
)
