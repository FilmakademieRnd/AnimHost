qt_add_library(AnimHostCore SHARED
    animhostcore.h animhostcore.cpp
    animhostnode.h animhostnode.cpp
    animhostinputnode.h animhostinputnode.cpp
    animhostoperationnode.h animhostoperationnode.cpp
    commondatatypes.h commondatatypes.cpp
    nodedatatypes.h
    sourcedatanode.h sourcedatanode.cpp
    animhosthelper.h animhosthelper.cpp
    ZMQMessageHandler.h ZMQMessageHandler.cpp
    UIUtils.h UIUtils.cpp

    animhostcore_global.h
)

set_target_properties (AnimHostCore PROPERTIES
    FOLDER Core
)

target_compile_definitions(AnimHostCore PRIVATE
    ANIMHOSTCORE_LIBRARY
)

target_link_libraries(AnimHostCore PUBLIC  
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    libzmq-mt-4_3_5
    QTNodes
)

target_include_directories(AnimHostCore INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(AnimHostCore PUBLIC
    ../../glm
    ../../nzmqt/3rdparty/cppzmq
    ../../nzmqt/include
    ./QTNodes/include
    ../animHost_Plugins/PluginInterface
    ../animHost_Plugins/PluginNodeInterface
    ./
)

target_link_directories(AnimHostCore PUBLIC 
    ./QTNodes/lib
    ../../nzmqt/3rdparty/zmq/lib
)



