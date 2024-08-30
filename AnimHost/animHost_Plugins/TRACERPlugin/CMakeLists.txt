set(target_name TRACERPlugin)

qt_add_library(${target_name}
    TRACERPlugin.cpp TRACERPlugin.h
    TRACERPlugin_global.h
    SceneReceiver/SceneReceiverNode.h SceneReceiver/SceneReceiverNode.cpp
    SceneReceiver/SceneReceiver.h SceneReceiver/SceneReceiver.cpp

    AnimationSender/AnimationSenderNode.h AnimationSender/AnimationSenderNode.cpp
    AnimationSender/AnimHostMessageSender.h AnimationSender/AnimHostMessageSender.cpp
    AnimationSender/TickReceiver.h AnimationSender/TickReceiver.cpp

    CharacterSelector/CharacterSelectorNode.h CharacterSelector/CharacterSelectorNode.cpp
    ControlPathUpdate/ControlPathUpdateNode.h ControlPathUpdate/ControlPathUpdateNode.cpp
    RPCTrigger/RPCTriggerNode.h RPCTrigger/RPCTriggerNode.cpp

)

set_target_properties (${target_name} PROPERTIES
    FOLDER Plugins/TRACER
)


target_include_directories(${target_name} PUBLIC
)

target_compile_definitions(${target_name} PRIVATE
    TRACERPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)